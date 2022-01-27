#ifndef WHIST_NETWORK_THROTTLE_H
#define WHIST_NETWORK_THROTTLE_H

#include <stdbool.h>

#ifndef _WIN32
#include <sys/types.h>
#endif  // _WIN32

typedef struct NetworkThrottleContext NetworkThrottleContext;

/**
 * @brief                    Initialize a new network throttler.
 *
 * @param coin_bucket_ms     The size of the coin bucket in milliseconds.
 * @param fill_bucket_initially    Whether the coin bucket should be filled up initially.
 *
 * @return                   The created network throttler context.
 */
NetworkThrottleContext* network_throttler_create(double coin_bucket_ms, bool fill_bucket_initially);

/**
 * @brief                    Destroy a network throttler.
 *
 * @param ctx                The network throttler context to destroy.
 */
void network_throttler_destroy(NetworkThrottleContext* ctx);

/**
 * @brief                    Set the bandwidth for the network throttler.
 *
 * @param ctx                The network throttler context.
 * @param burst_bitrate      The burst bandwidth in bits per second.
 */
void network_throttler_set_burst_bitrate(NetworkThrottleContext* ctx, int burst_bitrate);

/**
 * @brief                    Block the current thread until the network
 *                           throttler can accept more data.
 *
 * @param ctx                The network throttler context.
 * @param bytes              The number of bytes that will be sent.
 */
int network_throttler_wait_byte_allocation(NetworkThrottleContext* ctx, size_t bytes);

#endif  // WHIST_NETWORK_THROTTLE_H
