/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file features.h
 * @brief Feature flag handling.
 */

#include "whist/logging/logging.h"
#include "features.h"

typedef struct {
    /**
     * Feature which this structure describes.
     */
    WhistFeature feature;
    /**
     * Whether this feature is enabled when starting.
     */
    bool enabled;
    /**
     * String name for the feature.
     */
    const char *name;

} WhistFeatureDescriptor;

static WhistFeatureDescriptor feature_list[] = {
    {
        .feature = WHIST_FEATURE_PACKET_ENCRYPTION,
        .enabled = true,
        .name = "packet encryption",
    },
    {
        .feature = WHIST_FEATURE_LONG_TERM_REFERENCE_FRAMES,
        .enabled = false,
        .name = "long-term reference frames",
    },
};

static const WhistFeatureDescriptor *get_feature_descriptor(WhistFeature feature) {
    if (feature >= 0 && feature < WHIST_FEATURE_COUNT)
        return &feature_list[feature];
    else
        return NULL;
}

static uint32_t current_feature_mask;

void whist_init_features(void) {
    current_feature_mask = 0;
    for (int f = 0; f < WHIST_FEATURE_COUNT; f++) {
        const WhistFeatureDescriptor *desc = get_feature_descriptor(f);
        if (desc->enabled) {
            current_feature_mask |= 1 << f;
            LOG_INFO("Feature %s is enabled.", desc->name);
        } else {
            LOG_INFO("Feature %s is disabled.", desc->name);
        }
    }
}

bool whist_check_feature(WhistFeature feature) {
    FATAL_ASSERT(feature >= 0 && feature < WHIST_FEATURE_COUNT);

    if (current_feature_mask & (1 << feature)) {
        return true;
    } else {
        return false;
    }
}

void whist_set_feature(WhistFeature feature, bool value) {
    const WhistFeatureDescriptor *desc = get_feature_descriptor(feature);
    FATAL_ASSERT(desc);

    if (value) {
        LOG_INFO("Enabling %s feature.", desc->name);
        current_feature_mask |= 1 << feature;
    } else {
        LOG_INFO("Disabling %s feature.", desc->name);
        current_feature_mask &= ~(1 << feature);
    }
}

void whist_apply_feature_mask(uint32_t feature_mask) {
    LOG_INFO("Applying feature mask %08x.", feature_mask);

    if (current_feature_mask == feature_mask) {
        // No change.
        return;
    }

    for (unsigned int f = 0; f < WHIST_FEATURE_COUNT; f++) {
        if ((current_feature_mask & 1 << f) == (feature_mask & 1 << f)) {
            // This flag matches.
            continue;
        }
        const WhistFeatureDescriptor *desc = get_feature_descriptor(f);
        if (feature_mask & 1 << f) {
            LOG_INFO("Enabling %s feature.", desc->name);
            current_feature_mask |= 1 << f;
        } else {
            LOG_INFO("Disabling %s feature.", desc->name);
            current_feature_mask &= ~(1 << f);
        }
    }

    FATAL_ASSERT(current_feature_mask == feature_mask && "New unknown feature flags are set.");
}

uint32_t whist_get_feature_mask(void) { return current_feature_mask; }
