#include <whist/core/whist.h>
#include <whist/utils/clock.h>
#include <whist/utils/threads.h>
#include <whist/logging/log_statistic.h>
#include "throttle.h"
#include <server/server_statistic.h>

#define BITS_IN_BYTE 8.0

typedef struct NetworkThrottleContextInternal {
    double coin_bucket_ms;   //<<< The size of the coin bucket in milliseconds.
    size_t coin_bucket_max;  //<<< The maximum size of the coin bucket for the current burst bitrate
                             // and coin_bucket_ms.
    size_t coin_bucket;      //<<< The coin bucket for the current burst bitrate.
    int burst_bitrate;       //<<< The current burst bitrate.
    WhistMutex queue_lock;   //<<< The lock to protect the queue.
    WhistCondition queue_cond;      //<<< The condition variable which regulates the queue.
    clock coin_bucket_last_fill;    //<<< The timer for the coin bucket's last fill.
    unsigned int next_queue_id;     //<<< The next queue id to use.
    unsigned int current_queue_id;  //<<< The currently-processed queue id.
    bool destroying;                //<<< Whether the context is being destroyed.
    bool fill_bucket_initially;     //<<< Whether the coin bucket should be filled up initially
} NetworkThrottleContextInternal;

#define INTERNAL(ctx) ((NetworkThrottleContextInternal*)ctx->internal)

NetworkThrottleContext* network_throttler_create(double coin_bucket_ms,
                                                 bool fill_bucket_initially) {
    /*
        Initialize a new network throttler.

        Returns:
            (NetworkThrottleContext*): The created network throttler context.
    */
    NetworkThrottleContext* ctx = malloc(sizeof(NetworkThrottleContext));
    ctx->internal = malloc(sizeof(NetworkThrottleContextInternal));
    INTERNAL(ctx)->coin_bucket_ms = coin_bucket_ms;
    INTERNAL(ctx)->coin_bucket_max = (size_t)((INTERNAL(ctx)->coin_bucket_ms / MS_IN_SECOND) *
                                              (STARTING_BURST_BITRATE / BITS_IN_BYTE));
    if (fill_bucket_initially) {
        INTERNAL(ctx)->coin_bucket = INTERNAL(ctx)->coin_bucket_max;
    } else {
        INTERNAL(ctx)->coin_bucket = 0;
    }
    INTERNAL(ctx)->burst_bitrate = STARTING_BURST_BITRATE;
    INTERNAL(ctx)->queue_lock = whist_create_mutex();
    INTERNAL(ctx)->queue_cond = whist_create_cond();
    INTERNAL(ctx)->next_queue_id = 0;
    INTERNAL(ctx)->current_queue_id = 0;
    INTERNAL(ctx)->destroying = false;
    INTERNAL(ctx)->fill_bucket_initially = fill_bucket_initially;
    start_timer(&INTERNAL(ctx)->coin_bucket_last_fill);

    LOG_INFO("Created network throttler %p", ctx);

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

    INTERNAL(ctx)->destroying = true;

    while (INTERNAL(ctx)->current_queue_id != INTERNAL(ctx)->next_queue_id) {
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

    whist_destroy_mutex(INTERNAL(ctx)->queue_lock);
    whist_destroy_cond(INTERNAL(ctx)->queue_cond);
    free(ctx->internal);
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
        (size_t)((INTERNAL(ctx)->coin_bucket_ms / MS_IN_SECOND) * (burst_bitrate / BITS_IN_BYTE));

    // Add difference between previous max value and current max value to account for change in
    // bitrate
    if (INTERNAL(ctx)->fill_bucket_initially) {
        INTERNAL(ctx)->coin_bucket =
            max(INTERNAL(ctx)->coin_bucket + coin_bucket_max - INTERNAL(ctx)->coin_bucket_max, 0);
    }
    // We assume that only one thread is writing this at a time.
    // Multiple threads may read this concurrently. If we want
    // to support multiple threads, we need to lock a mutex around
    // accesses to `burst_bitrate`.
    INTERNAL(ctx)->burst_bitrate = burst_bitrate;
    INTERNAL(ctx)->coin_bucket_max = coin_bucket_max;
}

int network_throttler_get_burst_bitrate(NetworkThrottleContext* ctx) {
    /*
        Get the bandwidth for the network throttler.

        Arguments:
            ctx (NetworkThrottlerContext*): The network throttler context.

        Returns:
            (int): The burst bandwidth in bits per second.

        Note:
            This function is file-internal for the time being.
    */
    if (!ctx) return -1;

    return INTERNAL(ctx)->burst_bitrate;
}

void network_throttler_wait_byte_allocation(NetworkThrottleContext* ctx, size_t bytes) {
    /*
        Block the current thread until the network throttler can accept more data.

        Arguments:
            ctx (NetworkThrottlerContext*): The network throttler context.
            bytes (size_t): The number of bytes that will be sent.
    */
    if (!ctx || INTERNAL(ctx)->burst_bitrate <= 0 || INTERNAL(ctx)->destroying) return;

    whist_lock_mutex(INTERNAL(ctx)->queue_lock);
    unsigned int queue_id = INTERNAL(ctx)->next_queue_id++;
    while (queue_id > INTERNAL(ctx)->current_queue_id) {
        whist_wait_cond(INTERNAL(ctx)->queue_cond, INTERNAL(ctx)->queue_lock);
    }
    whist_unlock_mutex(INTERNAL(ctx)->queue_lock);

    // Now we have the guarantee that this is the next quued packet.
    // This thread now assumes the responsiblity of adding to the
    // coin bucket at a rate of `burst_bitrate` bytes per second.
    // Once there are enough coins in the bucket, we can actually
    // send the packet.
    clock start;
    start_timer(&start);
    int loops = 0;
    while (true) {
        ++loops;
        if (INTERNAL(ctx)->destroying) {
            // If we are in destruction mode, simply break
            break;
        }

        double elapsed_seconds = get_timer(INTERNAL(ctx)->coin_bucket_last_fill);
        start_timer(&INTERNAL(ctx)->coin_bucket_last_fill);
        int burst_bitrate = network_throttler_get_burst_bitrate(ctx);
        const size_t coin_bucket_max = INTERNAL(ctx)->coin_bucket_max;
        const size_t coin_bucket_update =
            (size_t)((double)elapsed_seconds * burst_bitrate / BITS_IN_BYTE);
        INTERNAL(ctx)->coin_bucket =
            min(INTERNAL(ctx)->coin_bucket + coin_bucket_update, coin_bucket_max);

        // We don't want to block the current thread forever if the packet is larger than
        // the max coin bucket, so cap it as well.
        size_t effective_bytes = min(bytes, coin_bucket_max);
        if (INTERNAL(ctx)->coin_bucket >= effective_bytes) {
            INTERNAL(ctx)->coin_bucket -= effective_bytes;
            break;
        }

        whist_usleep(50);
    }

    // Wake up the next waiter in the queue.
    double time = get_timer(start);
    log_double_statistic(NETWORK_THROTTLED_PACKET_DELAY, time * MS_IN_SECOND);
    log_double_statistic(NETWORK_THROTTLED_PACKET_DELAY_RATE, time * MS_IN_SECOND / (double)bytes);
    log_double_statistic(NETWORK_THROTTLED_PACKET_DELAY_LOOPS, (double)loops);
    ++INTERNAL(ctx)->current_queue_id;
    whist_lock_mutex(INTERNAL(ctx)->queue_lock);
    whist_broadcast_cond(INTERNAL(ctx)->queue_cond);
    whist_unlock_mutex(INTERNAL(ctx)->queue_lock);
}
