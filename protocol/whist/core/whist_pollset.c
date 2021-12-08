#include "whist_pollset.h"

#ifdef HAVE_POLL
#include <poll.h>
#endif

#include <assert.h>

#define POLLSET_STATIC_ITEMS 10

/**
 * Whist pollset is a component around poll/select, it allows to register a callback to invoke
 * when a socket becomes read or write ready.
 *
 * The implementation tries to avoid malloc() so it uses a static sets of items and when
 * these static sets are full, we switch to dynamically allocated ones. As we don't expect many
 * polled file descriptors, this code path shall never be used and we should stay on the small
 * static array.
 *
 * When adding a socket to poll for some event, we return a WhistPollsetSource, an integer,
 * which is an index in pollset->items. As we may remove and add socket to be polled, pollset->items
 * may have some holes in it, that's why we also maintain pollset->packedItems which is an array
 * of index of all active items in pollset->items. So internally browsing pollset->packedItems,
 * allows to treat all active items (as the most common operation is whist_pollset_poll() which
 * does that all the time).
 *
 * Note: a file descriptor should not be added twice in a whist_pollset or the behaviour of
 * callbacks invocation is undefined
 *
 * The WhistPollset can poll sockets using either poll() (available on most UNIX/mac) or select().
 *
 * The poll() implementation maintains an array of struct pollfd that is mapped on
 * pollset->packedItems. So if we have pollset->packedItems[4] == 0, that means that item 0 will
 * have its corresponding pollfd will be pollset->pollfds[4].
 *
 * The select() implementation maintains a read fd_set and a write fd_set and does various
 * computations to do a select()  in whist_pollset_select()
 *
 * WhistPollset also have timeout feature so if we set a timeout on the polled event, we can have
 * the callback called with WHIST_POLLEVENT_TIMEOUT if no event arrived in the corresponding delay.
 * Setting the timeout to 0 disables this feature.
 */

/** @brief a polled item in a whist pollset */
typedef struct {
    bool used;
    SOCKET s;
    int mask;
    int timeout;
    clock timeout_timer;
    WhistPollsetCb cb;
    void *cb_data;
} WhistPollsetItem;

/** @brief a whist pollset */
struct WhistPollset {
#ifdef HAVE_POLL
    struct pollfd static_poll_fd[POLLSET_STATIC_ITEMS];
    struct pollfd *pollfds;
#else
    fd_set rset;
    int nread;
    fd_set wset;
    int nwrite;
    SOCKET max_fd;
#endif
    size_t static_packed_items[POLLSET_STATIC_ITEMS];
    size_t *packed_items;
    WhistPollsetItem *items;
    WhistPollsetItem static_items[POLLSET_STATIC_ITEMS];
    size_t nitems;
    size_t items_capacity;
};

WhistPollset *whist_pollset_new() {
    WhistPollset *ret = safe_zalloc(sizeof(*ret));

    ret->items = &ret->static_items[0];
    ret->packed_items = &ret->static_packed_items[0];
    ret->items_capacity = POLLSET_STATIC_ITEMS;

#ifdef HAVE_POLL
    ret->pollfds = &ret->static_poll_fd[0];
#else
    FD_ZERO(&ret->rset);
    FD_ZERO(&ret->wset);
#endif
    return ret;
}

#ifndef HAVE_POLL
static SOCKET recompute_max_fd(WhistPollset *pollset) {
    size_t used = 0;
    size_t i;
    SOCKET ret = 0;

    for (i = 0; (i < pollset->items_capacity) && (used < pollset->nitems); i++) {
        WhistPollsetItem *item = &pollset->items[i];
        if (item->used) {
            if (item->s > ret) ret = item->s;
            used++;
        }
    }
    return ret;
}
#else
static int compute_poll_mask(int whist_mask) {
    int ret = 0;

    if (whist_mask & WHIST_POLLEVENT_READ) ret |= POLLIN;
    if (whist_mask & WHIST_POLLEVENT_WRITE) ret |= POLLOUT;

    return ret;
}
#endif

WhistPollsetSource whist_pollset_add(WhistPollset *pollset, SOCKET s, int mask, int timeout,
                                     WhistPollsetCb cb, void *data) {
    WhistPollsetSource ret;

    assert(pollset);
    assert(cb);

    /* find a free entry */
    for (ret = 0; ret < (int)pollset->items_capacity; ret++) {
        if (!pollset->items[ret].used) break;
    }

    if (ret == (int)pollset->items_capacity) {
        /* time to resize allocated items (or move to a dynamic set) */
        WhistPollsetItem *src_items =
            (pollset->items == pollset->static_items) ? NULL : pollset->items;
        size_t *packed_items =
            (pollset->items == pollset->static_items) ? NULL : pollset->packed_items;

        pollset->items_capacity += POLLSET_STATIC_ITEMS;
        pollset->items = safe_realloc(src_items, pollset->items_capacity * sizeof(*pollset->items));
        pollset->packed_items =
            safe_realloc(packed_items, pollset->items_capacity * sizeof(*pollset->packed_items));

#ifdef HAVE_POLL
        struct pollfd *src_pollfd = src_items ? pollset->pollfds : NULL;
        pollset->pollfds =
            safe_realloc(src_pollfd, pollset->items_capacity * sizeof(*pollset->pollfds));
#endif

        if (!src_items) {
            memcpy(pollset->items, pollset->static_items, sizeof(pollset->static_items));
            memcpy(pollset->packed_items, pollset->static_packed_items,
                   sizeof(pollset->static_packed_items));
#ifdef HAVE_POLL
            memcpy(pollset->pollfds, &pollset->static_poll_fd, sizeof(pollset->static_poll_fd));
#endif
        }

        /* initialize newly allocated entries */
        memset(&pollset->items[ret], 0, POLLSET_STATIC_ITEMS * sizeof(*pollset->items));
    }
    pollset->packed_items[pollset->nitems] = ret;

    WhistPollsetItem *item = &pollset->items[ret];
    item->used = true;
    item->s = s;
    item->mask = mask;
    item->timeout = timeout;
    if (item->timeout) start_timer(&item->timeout_timer);
    item->cb = cb;
    item->cb_data = data;

#ifdef HAVE_POLL
    /* in the case of poll there's an exact match between pollset->pollfds and pollset->packedItems
     */
    int pollmask = 0;
    int poll_idx;
    struct pollfd *poll_entry = &pollset->pollfds[pollset->nitems];

    poll_entry->fd = s;
    poll_entry->events = compute_poll_mask(mask);
    poll_entry->revents = 0;
    pollset->nitems++;

#else
    if (mask & WHIST_POLLEVENT_READ) {
        pollset->nread++;
        FD_SET(s, &pollset->rset);
    }
    if (mask & WHIST_POLLEVENT_WRITE) {
        pollset->nwrite++;
        FD_SET(s, &pollset->wset);
    }

    pollset->nitems++;
    pollset->max_fd = recompute_max_fd(pollset);
#endif

    return ret;
}

WhistPollsetSource whist_pollset_update(WhistPollset *pollset, WhistPollsetSource idx, int new_mask,
                                        int new_timeout, WhistPollsetCb cb, void *data) {
    assert(pollset);
    assert(cb);
    assert((size_t)idx < pollset->items_capacity);

    WhistPollsetItem *item = &pollset->items[idx];
    assert(item->used);

    item->cb = cb;
    item->cb_data = data;
    item->timeout = new_timeout;
    if (item->timeout) start_timer(&item->timeout_timer);

#ifdef HAVE_POLL
    size_t pollfd_index;
    for (pollfd_index = 0; pollfd_index < pollset->nitems; pollfd_index++)
        if (pollset->packed_items[pollfd_index] == (size_t)idx) break;
    assert(pollfd_index < pollset->nitems);

    struct pollfd *poll_entry = &pollset->pollfds[pollfd_index];
    poll_entry->events = compute_poll_mask(new_mask);
#else
    if ((item->mask & WHIST_POLLEVENT_READ) != (new_mask & WHIST_POLLEVENT_READ)) {
        /* new mask is different, update counters and sets */
        if (new_mask & WHIST_POLLEVENT_READ) {
            FD_SET(item->s, &pollset->rset);
            pollset->nread++;
        } else {
            FD_CLR(item->s, &pollset->rset);
            pollset->nread--;
        }
    }

    if ((item->mask & WHIST_POLLEVENT_WRITE) != (new_mask & WHIST_POLLEVENT_WRITE)) {
        /* new mask is different, update counters and sets */
        if (new_mask & WHIST_POLLEVENT_WRITE) {
            FD_SET(item->s, &pollset->wset);
            pollset->nwrite++;
        } else {
            FD_CLR(item->s, &pollset->wset);
            pollset->nwrite--;
        }
    }
#endif

    item->mask = new_mask;
    return idx;
}

void whist_pollset_remove(WhistPollset *pollset, WhistPollsetSource *pidx) {
    assert(pollset);
    assert(pidx);
    WhistPollsetSource idx = *pidx;
    assert(idx != WHIST_INVALID_SOURCE);
    assert((size_t)idx < pollset->items_capacity);

    WhistPollsetItem *item = &pollset->items[idx];
    assert(item->used);

    item->used = false;

    /* cleanup the packedSet */
    size_t i;
    for (i = 0; i < pollset->nitems; i++) {
        if (pollset->packed_items[i] == (size_t)idx) break;
    }

    assert(i < pollset->nitems);
    if (i != pollset->nitems - 1) {
        /* fill the hole created in pollset->packedItems with the last item of pollset->packedItems
         */
        pollset->packed_items[i] = pollset->packed_items[pollset->nitems - 1];

#ifdef HAVE_POLL
        memcpy(&pollset->pollfds[i], &pollset->pollfds[pollset->nitems - 1],
               sizeof(*pollset->pollfds));
#endif
    }

    pollset->nitems--;

#ifndef HAVE_POLL
    if (item->mask & WHIST_POLLEVENT_READ) {
        FD_CLR(item->s, &pollset->rset);
        pollset->nread--;
    }

    if (item->mask & WHIST_POLLEVENT_WRITE) {
        FD_CLR(item->s, &pollset->wset);
        pollset->nwrite--;
    }
    pollset->max_fd = recompute_max_fd(pollset);
#endif

    *pidx = WHIST_INVALID_SOURCE;
}

int whist_pollset_poll(WhistPollset *pollset, int timeout) {
    assert(pollset);

    clock select_timer;
    int ret = 0;
    size_t i;

    /* first, do a first pass to compute the effective timeout, and possibly notify timeouts */
    for (i = 0; i < pollset->nitems; i++) {
        WhistPollsetItem *item = &pollset->items[pollset->packed_items[i]];

        if (item->timeout) {
            int item_value = get_timer(item->timeout_timer) * 1000;
            if (item_value >= item->timeout) {
                /* due date is passed, notify timeout and reset counter */
                item->cb((WhistPollsetSource)i, WHIST_POLLEVENT_TIMEOUT, item->cb_data);
                ret++;
                start_timer(&item->timeout_timer);
            } else if (item->timeout - item_value < timeout) {
                /* adjust timeout with the remaining time */
                timeout = item->timeout - item_value;
            }
        }
    }

    if (ret) return ret;

    // printf("effective timeout=%d\n", timeout);
    start_timer(&select_timer);

#ifdef HAVE_POLL
    int poll_ret = 0;
    do {
        int wait_timeout = timeout;

        if (timeout) {
            int elapsed = get_timer(select_timer) * 1000;
            if (elapsed >= timeout) {
                poll_ret = 0;
                break;
            }
            wait_timeout = timeout - elapsed;
        }

        poll_ret = poll(pollset->pollfds, pollset->nitems, wait_timeout);
    } while (poll_ret < 0 && whist_is_EINTR());

    if (poll_ret < 0) return poll_ret;

    // printf("poll ret=%d\n", poll_ret);

    ret = 0;
    for (i = 0; i < pollset->nitems; i++) {
        WhistPollsetItem *item = &pollset->items[pollset->packed_items[i]];

        int mask = 0;
        if (item->timeout) {
            int elapsed = get_timer(item->timeout_timer) * 1000;
            if (elapsed >= item->timeout) mask |= WHIST_POLLEVENT_TIMEOUT;
        }

        if (poll_ret) {
            struct pollfd *poll_entry = &pollset->pollfds[i];
            if ((item->mask & WHIST_POLLEVENT_READ) && (poll_entry->revents & POLLIN))
                mask |= WHIST_POLLEVENT_READ;
            if ((item->mask & WHIST_POLLEVENT_WRITE) && (poll_entry->revents & POLLOUT))
                mask |= WHIST_POLLEVENT_WRITE;
        }

        if (mask) {
            item->cb((WhistPollsetSource)i, mask, item->cb_data);
            if (item->timeout) start_timer(&item->timeout_timer);
            ret++;
        }
    }

    return ret;
#else
    fd_set *rset, *wset;
    fd_set res_rset, res_wset;
    int select_ret = 0;

    rset = pollset->nread ? &res_rset : NULL;
    if (rset) memcpy(rset, &pollset->rset, sizeof(res_rset));

    wset = pollset->nwrite ? &res_wset : NULL;
    if (wset) memcpy(wset, &pollset->wset, sizeof(res_wset));

    do {
        struct timeval tv;

        if (timeout) {
            int elapsed = get_timer(select_timer) * 1000;
            if (elapsed >= timeout) {
                select_ret = 0;
                break;
            }
            int remaining_timeout = (timeout - elapsed);

            tv.tv_sec = remaining_timeout / 1000;
            tv.tv_usec = (remaining_timeout % 1000) * 1000;
        }

        // printf("select()\n");
        select_ret = select(pollset->max_fd + 1, rset, wset, NULL, timeout ? &tv : NULL);
    } while (select_ret < 0 && whist_is_EINTR());

    if (select_ret < 0) {
        // printf("select ret=%d errno=%d\n", select_ret, errno);
        return select_ret;
    }

    // printf("select ret=%d\n", select_ret);
    ret = 0;
    for (i = 0; i < pollset->nitems; i++) {
        WhistPollsetItem *item = &pollset->items[pollset->packed_items[i]];

        int mask = 0;
        if (item->timeout) {
            int elapsed = get_timer(item->timeout_timer) * 1000;
            if (elapsed >= item->timeout) mask |= WHIST_POLLEVENT_TIMEOUT;
        }

        if (select_ret) {
            if ((item->mask & WHIST_POLLEVENT_READ) && FD_ISSET(item->s, &res_rset)) {
                mask |= WHIST_POLLEVENT_READ;
            }

            if ((item->mask & WHIST_POLLEVENT_WRITE) && FD_ISSET(item->s, &res_wset)) {
                mask |= WHIST_POLLEVENT_WRITE;
            }
        }

        if (mask) {
            item->cb((WhistPollsetSource)i, mask, item->cb_data);
            if (item->timeout) start_timer(&item->timeout_timer);
            ret++;
        }
    }

    return ret;
#endif
}

void whist_pollset_free(WhistPollset **ppollset) {
    assert(ppollset);

    WhistPollset *pollset = *ppollset;

    if (pollset->items != pollset->static_items) free(pollset->items);
    if (pollset->packed_items != pollset->static_packed_items) free(pollset->packed_items);
#ifdef HAVE_POLL
    if (pollset->pollfds != pollset->static_poll_fd) free(pollset->pollfds);
#endif
    free(pollset);
    *ppollset = NULL;
}
