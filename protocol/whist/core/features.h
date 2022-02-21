/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file features.h
 * @brief Feature flag support.
 */
#ifndef WHIST_CORE_FEATURES_H
#define WHIST_CORE_FEATURES_H

#include <stdbool.h>

/**
 * @defgroup feature_flags Feature Flags
 *
 * Whist API for feature flag management.
 *
 * @{
 */

/**
 * Enumeration of optional protocol features.
 *
 * Do not add things in the middle of this enum!  Add only at the end.
 */
typedef enum {
    /**
     * Encrypt packets sent over the network.
     */
    WHIST_FEATURE_PACKET_ENCRYPTION,
    /**
     * Use long-term reference frames in the protocol.
     *
     * This enables generation of long-term reference frames on the
     * server side and the sending of frame-ack messages on the client
     * side.
     */
    WHIST_FEATURE_LONG_TERM_REFERENCE_FRAMES,
    /**
     * Number of supported feature flags.
     *
     * Add new features before this entry.
     */
    WHIST_FEATURE_COUNT,
} WhistFeature;

/**
 * Initialise default features.
 *
 * Must be run before calling any other feature functions.
 */
void whist_init_features(void);

/**
 * Check whether a feature is enabled.
 *
 * @param feature  Feature to check.
 * @return  true if the feature is enabled, false if it isn't.
 */
bool whist_check_feature(WhistFeature feature);

/**
 * Set whether a feature is enabled.
 *
 * @param feature  Feature to set.
 * @param value    New value to set.
 */
void whist_set_feature(WhistFeature feature, bool value);

/**
 * Convenience macro for checking a feature.
 *
 * For example:
 * @code
 * if (FEATURE_ENABLED(PACKET_ENCRYPTION)) {
 *     // Do stuff with packet encryption.
 * }
 * @endcode
 */
#define FEATURE_ENABLED(name) (whist_check_feature(WHIST_FEATURE_##name))

/**
 * Apply a new feature mask.
 *
 * This is used on the client to apply a new mask received from the
 * server.  It will abort if the supplied mask is not compatible with
 * the current one.
 *
 * @param feature_mask  Feature mask to apply.
 */
void whist_apply_feature_mask(uint32_t feature_mask);

/**
 * Retrieve the current feature mask.
 *
 * This is used on the server to get the mask to send to the client.
 */
uint32_t whist_get_feature_mask(void);

/** @} */

#endif /* WHIST_CORE_FEATURES_H */
