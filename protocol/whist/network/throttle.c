#include <whist/core/whist.h>
#include <whist/utils/atomic.h>
#include <whist/utils/clock.h>
#include <whist/utils/threads.h>
#include <whist/logging/log_statistic.h>
#include "throttle.h"
#include <server/server_statistic.h>
#include <whist/network/network_algorithm.h>

// Set this to something very low. Throttler will work as expected only if
// network_throttler_set_burst_bitrate() is called with the required bitrate
#define STARTING_THROTTLER_BITRATE 1000000

struct NetworkThrottleContext {
    double coin_bucket_ms;   //<<< The size of the coin bucket in milliseconds.
    size_t coin_bucket_max;  //<<< The maximum size of the coin bucket for the current burst bitrate
                             // and coin_bucket_ms.
    size_t coin_bucket;      //<<< The coin bucket for the current burst bitrate.
    int burst_bitrate;       //<<< The current burst bitrate.
    WhistMutex queue_lock;   //<<< The lock to protect the queue.
    WhistCondition queue_cond;         //<<< The condition variable which regulates the queue.
    WhistTimer coin_bucket_last_fill;  //<<< The timer for the coin bucket's last fill.
    atomic_int next_queue_id;          //<<< The next queue id to use.
    atomic_int current_queue_id;       //<<< The currently-processed queue id.
    unsigned int group_id;       //<<< id of the group of packets being sent in the current burst.
    bool destroying;             //<<< Whether the context is being destroyed.
    bool fill_bucket_initially;  //<<< Whether the coin bucket should be filled up initially
};

NetworkThrottleContext* network_throttler_create(double coin_bucket_ms,
                                                 bool fill_bucket_initially) {
    /*
        Initialize a new network throttler.

        Returns:
            (NetworkThrottleContext*): The created network throttler context.
    */
    NetworkThrottleContext* ctx = safe_malloc(sizeof(NetworkThrottleContext));
    ctx->coin_bucket_ms = coin_bucket_ms;
    ctx->coin_bucket_max = (size_t)((ctx->coin_bucket_ms / MS_IN_SECOND) *
                                    (STARTING_THROTTLER_BITRATE / BITS_IN_BYTE));
    if (fill_bucket_initially) {
        ctx->coin_bucket = ctx->coin_bucket_max;
    } else {
        ctx->coin_bucket = 0;
    }
    ctx->burst_bitrate = STARTING_THROTTLER_BITRATE;
    ctx->queue_lock = whist_create_mutex();
    ctx->queue_cond = whist_create_cond();
    atomic_init(&ctx->next_queue_id, 0);
    atomic_init(&ctx->current_queue_id, 0);
    ctx->group_id = 0;
    ctx->destroying = false;
    ctx->fill_bucket_initially = fill_bucket_initially;
    start_timer(&ctx->coin_bucket_last_fill);

    return ctx;
}

void network_throttler_destroy(NetworkThrottleContext* ctx) {
    /*
        Destroy a network throttler.

        Arguments:
            ctx (NetworkThrottlerContext*): The network throttler context to destroy.
    */
    if (!ctx) return;

    LOG_INFO("Flushing queue for network throttler %p", ctx);

    ctx->destroying = true;

    while (atomic_load(&ctx->current_queue_id) != atomic_load(&ctx->next_queue_id)) {
        // Wait until the packet queue is empty
        whist_sleep(10);
    }

    LOG_INFO("Destroying and freeing network throttler %p", ctx);

    // At this point, we have guaranteed that no thread is waiting
    // on the condition variable and that no thread is currently
    // in charge of processing the queue. So we can safely destroy
    // everything. Note that technically, we should also ensure
    // that no thread is trying to set the burst bitrate at this
    // moment, but for now we just assume that the caller is
    // smart about that.

    whist_destroy_mutex(ctx->queue_lock);
    whist_destroy_cond(ctx->queue_cond);
    free(ctx);
}

void network_throttler_set_burst_bitrate(NetworkThrottleContext* ctx, int burst_bitrate) {
    /*
        Set the bandwidth for the network throttler.

        Arguments:
            ctx (NetworkThrottlerContext*): The network throttler context.
            burst_bitrate (int): The burst bandwidth in bits per second.
    */
    if (!ctx) return;

    size_t coin_bucket_max =
        (size_t)((ctx->coin_bucket_ms / MS_IN_SECOND) * (burst_bitrate / BITS_IN_BYTE));

    // Add difference between previous max value and current max value to account for change in
    // bitrate
    if (ctx->fill_bucket_initially) {
        ctx->coin_bucket = max(ctx->coin_bucket + coin_bucket_max - ctx->coin_bucket_max, 0);
    }
    // We assume that only one thread is writing this at a time.
    // Multiple threads may read this concurrently. If we want
    // to support multiple threads, we need to lock a mutex around
    // accesses to `burst_bitrate`.
    ctx->burst_bitrate = burst_bitrate;
    ctx->coin_bucket_max = coin_bucket_max;
}

int network_throttler_wait_byte_allocation(NetworkThrottleContext* ctx, size_t bytes) {
    /*
        Block the current thread until the network throttler can accept more data.

        Arguments:
            ctx (NetworkThrottlerContext*): The network throttler context.
            bytes (size_t): The number of bytes that will be sent.
    */
    if (!ctx || ctx->burst_bitrate <= 0 || ctx->destroying) return -1;

    int queue_id = atomic_fetch_add(&ctx->next_queue_id, 1);
    whist_lock_mutex(ctx->queue_lock);
    while (queue_id > atomic_load(&ctx->current_queue_id)) {
        whist_wait_cond(ctx->queue_cond, ctx->queue_lock);
    }
    whist_unlock_mutex(ctx->queue_lock);

    // Now we have the guarantee that this is the next quued packet.
    // This thread now assumes the responsiblity of adding to the
    // coin bucket at a rate of `burst_bitrate` bytes per second.
    // Once there are enough coins in the bucket, we can actually
    // send the packet.
    WhistTimer start;
    start_timer(&start);
    do {
        if ((get_timer(&ctx->coin_bucket_last_fill) * MS_IN_SECOND) > ctx->coin_bucket_ms) {
            // If the previous bucket is almost consumed(less than one UDP packet available), then
            // add remaining coins to the next bucket. Otherwise ignore the remaining coins.
            if (ctx->coin_bucket < udp_packet_max_size())
                ctx->coin_bucket += ctx->coin_bucket_max;
            else
                ctx->coin_bucket = ctx->coin_bucket_max;
            ctx->group_id++;
            start_timer(&ctx->coin_bucket_last_fill);
        } else if (bytes > ctx->coin_bucket) {
            whist_usleep((ctx->coin_bucket_ms * US_IN_MS) -
                         (get_timer(&ctx->coin_bucket_last_fill) * US_IN_SECOND));
        }
    } while (bytes > ctx->coin_bucket);
    ctx->coin_bucket -= bytes;

    // Wake up the next waiter in the queue.
    double time = get_timer(&start);
    log_double_statistic(NETWORK_THROTTLED_PACKET_DELAY, time * MS_IN_SECOND);
    log_double_statistic(NETWORK_THROTTLED_PACKET_DELAY_RATE, time * MS_IN_SECOND / (double)bytes);
    atomic_fetch_add(&ctx->current_queue_id, 1);
    whist_lock_mutex(ctx->queue_lock);
    whist_broadcast_cond(ctx->queue_cond);
    whist_unlock_mutex(ctx->queue_lock);
    return ctx->group_id;
}
