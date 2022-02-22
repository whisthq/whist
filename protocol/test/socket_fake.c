/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file socket_fake.c
 * @brief Fake sockets implementation for socket API tests.
 */

#include "whist/core/whist.h"
#include "whist/utils/atomic.h"
#include "whist/utils/linked_list.h"
#include "whist/network/socket.h"
#include "whist/network/socket_internal.h"
#include "socket_fake.h"

typedef struct FakeSocketContext {
    LINKED_LIST_HEADER;

    bool is_bound;
    FakeSocketAddress bind_addr;

    bool is_connected;
    FakeSocketAddress connect_addr;

    bool non_blocking;

    bool pending_timeout;
    timestamp_us timeout_target;

    LinkedList receive_queue;

    WhistMutex mutex;
    WhistCondition cond;
} FakeSocketContext;

struct FakeNetworkContext {
    // Next unique packet ID to use.
    atomic_int unique_packet_id;
    // Flag to signal event thread to finish.
    bool finish;

    // Pending packets (enqueued by send()).
    LinkedList pending_packet_list;
    // In-flight packets, in arrival order.
    LinkedList inflight_packet_list;
    // Active bound sockets.
    LinkedList socket_list;

    // Network thread handle.
    WhistThread thread;
    // Mutex protecting everything else in this structure.
    WhistMutex mutex;
    // Condition for waking the network thread.
    WhistCondition cond;

    // Callback for user control of how the network works.
    // Edits packet fields, or returns true if packet should be dropped.
    FakeNetworkPacketHandler user_handler;
    // Argument for user_handler callback.
    void *user_arg;
};

static bool fake_socket_address_match(const FakeSocketAddress *a, const FakeSocketAddress *b) {
    if (a->len != b->len)
        return false;
    if (a->addr.sa_family != b->addr.sa_family)
        return false;
    if (a->addr.sa_family == AF_INET) {
        const struct sockaddr_in *addr_a = (const struct sockaddr_in*)&a->addr,
                                 *addr_b = (const struct sockaddr_in*)&b->addr;
        if (addr_a->sin_port != addr_b->sin_port)
            return false;
        return (addr_a->sin_addr.s_addr == addr_b->sin_addr.s_addr ||
                addr_a->sin_addr.s_addr == INADDR_ANY ||
                addr_b->sin_addr.s_addr == INADDR_ANY);
    } else {
        return !memcmp(&a->addr, &b->addr, a->len);
    }
}

static void fake_socket_generate_ephemeral_address(FakeSocketAddress *addr) {
    // Avoid low port numbers.
    int port = rand() % 60000 + 2000;
    struct sockaddr_in tmp = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = 0x78563412,
    };
    memcpy(&addr->addr, &tmp, sizeof(tmp));
    addr->len = sizeof(tmp);
}

static FakePacket *fake_packet_alloc(FakeNetworkContext *net, size_t len) {
    FakePacket *pkt;
    pkt = safe_malloc(sizeof(*pkt));
    memset(pkt, 0, sizeof(*pkt));
    pkt->id = atomic_fetch_add(&net->unique_packet_id, 1);
    pkt->data = safe_malloc(len);
    pkt->len = len;
    pkt->send_time = current_time_us();
    return pkt;
}

static void fake_packet_free(FakePacket *pkt) {
    free(pkt->data);
    free(pkt);
}

static void fake_network_deliver_packet(FakeNetworkContext *net, FakePacket *pkt) {
    bool delivered = false;
    linked_list_for_each(&net->socket_list, FakeSocketContext, sock) {
        whist_lock_mutex(sock->mutex);
        if (fake_socket_address_match(&pkt->dst_addr, &sock->bind_addr) &&
            (!sock->is_connected ||
             fake_socket_address_match(&pkt->src_addr, &sock->connect_addr))) {
            LOG_INFO("Packet %d delivered to socket.", pkt->id);
            linked_list_add_tail(&sock->receive_queue, pkt);
            whist_broadcast_cond(sock->cond);
            delivered = true;
        }
        whist_unlock_mutex(sock->mutex);
        if (delivered)
            break;
    }
    if (!delivered) {
        LOG_INFO("Packet %d dropped because noone was listening for it.", pkt->id);
        fake_packet_free(pkt);
    }
}

static void fake_network_send_packet(FakeNetworkContext *net, FakePacket *pkt) {
    // Call user network callback to decide whether the packet gets
    // through, and if so how long it takes.
    bool drop = net->user_handler(net->user_arg, pkt);
    if (drop) {
        LOG_INFO("Packet %d dropped due to network.", pkt->id);
        fake_packet_free(pkt);
        return;
    }

    // Packets can't arrive before they are sent.
    FATAL_ASSERT(pkt->arrival_time >= pkt->send_time);

    LOG_INFO("Packet %d will be in-flight for %"PRIu64"us.", pkt->id, pkt->arrival_time - pkt->send_time);

    // Add to the inflight list in the right place.
    linked_list_for_each(&net->inflight_packet_list, FakePacket, iter) {
        if (pkt->arrival_time < iter->arrival_time) {
            linked_list_add_before(&net->inflight_packet_list, iter, pkt);
            return;
        }
    }
    linked_list_add_tail(&net->inflight_packet_list, pkt);
}

static int fake_network_thread_function(void *arg) {
    FakeNetworkContext *net = arg;

    whist_lock_mutex(net->mutex);
    while (!net->finish) {
        FakePacket *pkt;
        while (pkt = linked_list_extract_head(&net->pending_packet_list)) {
            LOG_INFO("Network thread sending packet %d.", pkt->id);
            fake_network_send_packet(net, pkt);
        }

        timestamp_us now = current_time_us();
        while (pkt = linked_list_head(&net->inflight_packet_list)) {
            if (pkt->arrival_time > now) {
                break;
            }
            LOG_INFO("Network thread delivering packet %d.", pkt->id);
            linked_list_remove(&net->inflight_packet_list, pkt);
            fake_network_deliver_packet(net, pkt);
        }

        timestamp_us wait_target = 0;
        if (pkt) {
            wait_target = pkt->arrival_time;
        }

        linked_list_for_each(&net->socket_list, FakeSocketContext, sock) {
            whist_lock_mutex(sock->mutex);
            if (sock->pending_timeout) {
                if (sock->timeout_target <= now) {
                    LOG_INFO("Network thread waking socket on timeout.");
                    sock->pending_timeout = false;
                    whist_broadcast_cond(sock->cond);
                } else if (!wait_target || sock->timeout_target < wait_target) {
                    wait_target = sock->timeout_target;
                }
            }
            whist_unlock_mutex(sock->mutex);
        }

        if (wait_target == 0) {
            LOG_INFO("Network thread waiting for next event.");
            whist_wait_cond(net->cond, net->mutex);
        } else {
            int64_t wait_us = (int64_t)wait_target - (int64_t)current_time_us();
            if (wait_us >= 0) {
                // Round up to avoid spinning.
                int wait_ms = (wait_us + 999) / 1000;
                if (wait_ms == 0) wait_ms = 1;

                LOG_INFO("Network thread waiting %dms (%"PRId64"us) for next event.",
                         wait_ms, wait_us);
                whist_wait_cond_timeout(net->cond, net->mutex, wait_ms);
            }
        }
    }
    whist_unlock_mutex(net->mutex);

    return 0;
}

static bool fake_network_default_callback(void *arg, FakePacket *pkt) {
    // All packets get through with 5ms delay (10ms round-trip).
    pkt->arrival_time = pkt->send_time + 10000;
    return false;
}

FakeNetworkContext *fake_network_create(void) {
    FakeNetworkContext *net;
    net = safe_malloc(sizeof(*net));

    atomic_init(&net->unique_packet_id, 1);
    net->finish = false;

    linked_list_init(&net->pending_packet_list);
    linked_list_init(&net->inflight_packet_list);
    linked_list_init(&net->socket_list);

    net->mutex = whist_create_mutex();
    net->cond = whist_create_cond();

    net->user_handler = &fake_network_default_callback;
    net->user_arg = NULL;

    net->thread = whist_create_thread(&fake_network_thread_function, "Fake Network", net);
    FATAL_ASSERT(net->thread);

    return net;
}

void fake_network_destroy(FakeNetworkContext *net) {
    whist_lock_mutex(net->mutex);
    net->finish = true;
    whist_broadcast_cond(net->cond);
    whist_unlock_mutex(net->mutex);

    whist_wait_thread(net->thread, NULL);

    linked_list_for_each(&net->pending_packet_list, FakePacket, pkt) {
        fake_packet_free(pkt);
    }
    linked_list_for_each(&net->inflight_packet_list, FakePacket, pkt) {
        fake_packet_free(pkt);
    }

    if (linked_list_size(&net->socket_list) > 0) {
        LOG_WARNING("Destroying fake network with %d sockets active.",
                    linked_list_size(&net->socket_list));
    }

    whist_destroy_mutex(net->mutex);
    whist_destroy_cond(net->cond);

    free(net);
}

void fake_network_set_packet_handler(FakeNetworkContext *net,
                                     FakeNetworkPacketHandler handler, void *user) {
    net->user_handler = handler;
    net->user_arg = user;
}

// This global is used when a socket is created with a network context.
static FakeNetworkContext *fake_network_default;

static void fake_socket_global_init(void) {
    fake_network_default = fake_network_create();
}

static void fake_socket_global_destroy(void) {
    fake_network_destroy(fake_network_default);
}

static void fake_socket_wait(WhistSocket *sock) {
    FakeSocketContext *ctx = sock->context;
    FakeNetworkContext *net = sock->network;

    if (sock->options[SOCKET_OPTION_NON_BLOCKING]) {
        // Non-blocking sockets do not wait.
        return;
    }

    LOG_INFO("Wait for packet arrival.");

    if (sock->options[SOCKET_OPTION_RECEIVE_TIMEOUT]) {
        // Mutex ordering.
        whist_unlock_mutex(ctx->mutex);
        whist_lock_mutex(net->mutex);
        whist_lock_mutex(ctx->mutex);

        ctx->timeout_target = current_time_us() + 1000 * sock->options[SOCKET_OPTION_RECEIVE_TIMEOUT];
        ctx->pending_timeout = true;

        whist_broadcast_cond(net->cond);
        whist_unlock_mutex(net->mutex);

        while (linked_list_size(&ctx->receive_queue) == 0 &&
               ctx->pending_timeout)
            whist_wait_cond(ctx->cond, ctx->mutex);
    } else {
        while (linked_list_size(&ctx->receive_queue) == 0)
            whist_wait_cond(ctx->cond, ctx->mutex);
    }
}

static int fake_socket_add(WhistSocket *sock) {
    FakeSocketContext *ctx = sock->context;
    FakeNetworkContext *net = sock->network;
    int ret;

    whist_lock_mutex(net->mutex);

    while (1) {
        bool in_use = false;
        if (!ctx->is_bound) {
            fake_socket_generate_ephemeral_address(&ctx->bind_addr);
        }
        linked_list_for_each(&net->socket_list, FakeSocketContext, iter) {
            if (fake_socket_address_match(&iter->bind_addr, &ctx->bind_addr)) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            ctx->is_bound = true;
            LOG_INFO("Add socket to bound list.");
            linked_list_add_tail(&net->socket_list, ctx);
            ret = 0;
            break;
        }
        if (ctx->is_bound) {
            ret = -1;
            break;
        }
    }

    whist_unlock_mutex(net->mutex);

    return ret;
}

static void fake_socket_remove(WhistSocket *sock) {
    FakeSocketContext *ctx = sock->context;
    FakeNetworkContext *net = sock->network;

    whist_lock_mutex(net->mutex);
    LOG_INFO("Remove socket from bound list.");
    linked_list_remove(&net->socket_list, ctx);
    whist_unlock_mutex(net->mutex);
}

static int fake_socket_create(WhistSocket *sock) {
    FakeSocketContext *ctx = sock->context;

    if (!sock->network) {
        LOG_INFO("Fake socket created using default network.");
        sock->network = fake_network_default;
    }

    ctx->mutex = whist_create_mutex();
    ctx->cond = whist_create_cond();

    return 0;
}

static void fake_socket_close(WhistSocket *sock) {
    FakeSocketContext *ctx = sock->context;

    if (ctx->is_bound) {
        fake_socket_remove(sock);
    }

    if (linked_list_size(&ctx->receive_queue) > 0) {
        LOG_INFO("Discarding %d packets from receive queue when closing socket %d.",
                 linked_list_size(&ctx->receive_queue), sock->id);
        linked_list_for_each(&ctx->receive_queue, FakePacket, pkt) {
            fake_packet_free(pkt);
        }
    }

    whist_destroy_mutex(ctx->mutex);
    whist_destroy_cond(ctx->cond);
}

static int fake_socket_set_option(WhistSocket *sock, WhistSocketOption opt, int value) {
    sock->options[opt] = value;
    return 0;
}

static int fake_socket_bind(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen) {
    FakeSocketContext *ctx = sock->context;
    int ret;

    ctx->bind_addr.addr = *addr;
    ctx->bind_addr.len = addrlen;
    ctx->is_bound = true;

    ret = fake_socket_add(sock);
    if (ret < 0) {
        sock->last_error = SOCKET_ERROR_ADDRESS_IN_USE;
        return -1;
    } else {
        return 0;
    }
}

static int fake_socket_connect(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen) {
    FakeSocketContext *ctx = sock->context;

    ctx->connect_addr.addr = *addr;
    ctx->connect_addr.len = addrlen;
    ctx->is_connected = true;

    return 0;
}

static int fake_socket_sendto(WhistSocket *sock, const void *buf, size_t len, int flags,
                              const struct sockaddr *dest_addr, socklen_t  addrlen) {
    FakeSocketContext *ctx = sock->context;
    FakeNetworkContext *net = sock->network;
    FakePacket *pkt;
    int ret;

    if (!ctx->is_bound) {
        // Generate a source address to send from.
        ret = fake_socket_add(sock);
        if (ret < 0) {
            sock->last_error = SOCKET_ERROR_UNKNOWN;
            return -1;
        }
    }

    pkt = fake_packet_alloc(net, len);
    memcpy(pkt->data, buf, len);

    pkt->src_addr = ctx->bind_addr;
    pkt->dst_addr.addr = *dest_addr;
    pkt->dst_addr.len = addrlen;

    whist_lock_mutex(net->mutex);
    linked_list_add_tail(&net->pending_packet_list, pkt);
    whist_broadcast_cond(net->cond);
    whist_unlock_mutex(net->mutex);

    return (int)len;
}

static int fake_socket_send(WhistSocket *sock, const void *buf, size_t len, int flags) {
    FakeSocketContext *ctx = sock->context;
    if (!ctx->is_connected) {
        sock->last_error = SOCKET_ERROR_NOT_CONNECTED;
        return -1;
    }
    return fake_socket_sendto(sock, buf, len, flags,
                              &ctx->connect_addr.addr, ctx->connect_addr.len);
}

static int fake_socket_recvfrom(WhistSocket *sock, void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen) {
    FakeSocketContext *ctx = sock->context;
    int ret;

    whist_lock_mutex(ctx->mutex);

    if (linked_list_size(&ctx->receive_queue) == 0) {
        fake_socket_wait(sock);
    }

    FakePacket *pkt = linked_list_extract_head(&ctx->receive_queue);
    if (pkt) {
        if (len > pkt->len)
            len = pkt->len;
        memcpy(buf, pkt->data, len);
        if (src_addr) {
            memcpy(src_addr, &pkt->src_addr.addr, pkt->src_addr.len);
            *addrlen = pkt->src_addr.len;
        }
        ret = (int)len;
        fake_packet_free(pkt);
    } else {
        sock->last_error = SOCKET_ERROR_WOULD_BLOCK;
        ret = -1;
    }

    whist_unlock_mutex(ctx->mutex);
    return ret;
}

static int fake_socket_recv(WhistSocket *sock, void *buf, size_t len, int flags) {
    return fake_socket_recvfrom(sock, buf, len, flags, NULL, NULL);
}

const WhistSocketImplementation socket_implementation_fake_udp = {
    .name = "Fake UDP socket",
    .type = SOCKET_TYPE_UDP,
    .context_size = sizeof(FakeSocketContext),

    .global_init = &fake_socket_global_init,
    .global_destroy = &fake_socket_global_destroy,

    .create = &fake_socket_create,
    .close = &fake_socket_close,

    .set_option = &fake_socket_set_option,

    .bind = &fake_socket_bind,
    .connect = &fake_socket_connect,

    .send = &fake_socket_send,
    .sendto = &fake_socket_sendto,
    .recv = &fake_socket_recv,
    .recvfrom = &fake_socket_recvfrom,
};
