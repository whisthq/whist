#ifndef PROTOCOL_WHIST_CORE_WHIST_POLLSET_H_
#define PROTOCOL_WHIST_CORE_WHIST_POLLSET_H_

#include <whist/network/network.h>

/** @brief kind of event */
typedef enum {
    WHIST_POLLEVENT_READ = 1,
    WHIST_POLLEVENT_WRITE = 2,
    WHIST_POLLEVENT_TIMEOUT = 4
} WhistPollsetEvent;

typedef int WhistPollsetSource;

#define WHIST_INVALID_SOURCE ((WhistPollsetSource)(-1))

/** @brief callback called during whist_pollset_poll()
 * @param idx the event source that triggered that callback
 * @param mask the kind of event that is notified
 * @param data data associated with the callback when calling whist_pollset_add()
 */
typedef void (*WhistPollsetCb)(WhistPollsetSource idx, int mask, void *data);

struct WhistPollset;
typedef struct WhistPollset WhistPollset;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * creates and initialize a new pollset that should be freed using whist_pollset_free
 * @return the created pollset
 */
WhistPollset *whist_pollset_new(void);

/**
 * Adds a socket to be polled in this pollset. mask gives what kind of activity we want to poll,
 * cb and data are the callback and data callback that are invoked when some activity is detected.
 *
 * @param pollset target pollset
 * @param s socket to poll
 * @param mask the events to poll (a ORed of WHIST_POLLEVENT_READ and WHIST_POLLEVENT_WRITE)
 * @param timeout a timeout in milliseconds, set to 0 if you don't want timeout notifications
 * @param cb the callback to call when an event is notified
 * @param data the data pointer to pass to the callback
 * @return a WhistPollsetSource to use when manipulating sources, or WHIST_INVALID_SOURCE if
 * something failed
 */
WhistPollsetSource whist_pollset_add(WhistPollset *pollset, SOCKET s, int mask, int timeout,
                                     WhistPollsetCb cb, void *data);

/**
 * Updates the settings of an existing event source
 *
 * @param pollset the pollset
 * @param idx the event source
 * @param mask new mask for events (same meaning as the mask parameter in whist_pollset_add)
 * @param timeout same meaning as the timeout parameter in whist_pollset_add
 * @param cb same meaning as the cb parameter in whist_pollset_add
 * @param data same meaning as the data parameter in whist_pollset_add
 * @return shall return idx if successfull and WHIST_INVALID_SOURCE otherwise
 */
WhistPollsetSource whist_pollset_update(WhistPollset *pollset, WhistPollsetSource idx, int mask,
                                        int timeout, WhistPollsetCb cb, void *data);

/**
 * Removes an event source from the pollset
 * @param pollset the pollset
 * @param pidx pointer on event source, it is set to WHIST_INVALID_SOURCE after the call
 */
void whist_pollset_remove(WhistPollset *pollset, WhistPollsetSource *pidx);

/**
 *	Does a polling loop of maximum timeout milliseconds and deliver events. A 0 timeout
 *	means wait forever until an event happens. Please note that the code computes the
 *	effective wait time	based on event sources timeouts if any, so you may wait a lot less
 *	than timeout.
 *
 * @param pollset the pollset to poll
 * @param timeout the maximum timeout, set to 0 for no timeout
 * @return the number of treated event (including timeout notifications)
 */
int whist_pollset_poll(WhistPollset *pollset, int timeout);

/**
 * Disposes a pollset releasing all its entries. The pollset and its content are freed
 * and the pointer nullified.
 *
 * @param ppollset the address of a pointer WhistPollset to remove
 */
void whist_pollset_free(WhistPollset **ppollset);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_WHIST_CORE_WHIST_POLLSET_H_ */
