/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file features.h
 * @brief Feature flag handling.
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "whist/logging/logging.h"
#include "whist/utils/command_line.h"
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
        .enabled = true,
        .name = "long-term reference frames",
    },
};

static const WhistFeatureDescriptor *get_feature_descriptor(WhistFeature feature) {
    if (feature >= 0 && feature < WHIST_FEATURE_COUNT)
        return &feature_list[feature];
    else
        return NULL;
}

static const WhistFeatureDescriptor *match_feature_name(const char *str, size_t len) {
    // Match strings while ignoring case and non-alphabetic characters.
    for (int f = 0; f < WHIST_FEATURE_COUNT; f++) {
        const WhistFeatureDescriptor *feature = &feature_list[f];
        bool match = true;
        size_t s = 0, d = 0;
        while (s < len) {
            if (tolower(str[s]) != feature->name[d]) {
                match = false;
                break;
            }
            // Find the next alphabetic character in each string.
            do
                ++s;
            while (s < len && str[s] && !isalpha(str[s]));
            do
                ++d;
            while (feature->name[d] && !isalpha(feature->name[d]));
        }
        if (match) {
            return feature;
        }
    }
    return NULL;
}

static uint32_t command_line_override_values;
static uint32_t command_line_override_mask;

static bool command_line_feature_list(const char *value, bool enable) {
    const WhistFeatureDescriptor *feature;
    const char *start = value, *end;
    size_t len;
    while (1) {
        end = strchr(start, ',');
        if (end)
            len = end - start;
        else
            len = strlen(start);

        feature = match_feature_name(start, len);
        if (feature) {
            command_line_override_values |= enable << feature->feature;
            command_line_override_mask |= 1 << feature->feature;
        } else {
            printf("Unable to enable unknown feature \"%.*s\".\n", (int)len, start);
            return false;
        }

        if (end)
            start = end + 1;
        else
            break;
    }
    return true;
}

static bool enable_features(const WhistCommandLineOption *opt, const char *value) {
    return command_line_feature_list(value, true);
}

static bool disable_features(const WhistCommandLineOption *opt, const char *value) {
    return command_line_feature_list(value, false);
}

COMMAND_LINE_CALLBACK_OPTION(enable_features, 0, "enable-features", WHIST_OPTION_REQUIRED_ARGUMENT,
                             "List of features to enable, comma-separated.")
COMMAND_LINE_CALLBACK_OPTION(disable_features, 0, "disable-features",
                             WHIST_OPTION_REQUIRED_ARGUMENT,
                             "List of features to disable, comma-separated.")

static uint32_t current_feature_mask;

void whist_init_features(void) {
    current_feature_mask = 0;
    for (int f = 0; f < WHIST_FEATURE_COUNT; f++) {
        const WhistFeatureDescriptor *desc = get_feature_descriptor(f);
        if ((1 << f) & command_line_override_mask) {
            if ((1 << f) & command_line_override_values) {
                current_feature_mask |= 1 << f;
                LOG_INFO("Feature %s is enabled from the command line.", desc->name);
            } else {
                LOG_INFO("Feature %s is disabled from the command line.", desc->name);
            }
        } else {
            if (desc->enabled) {
                current_feature_mask |= 1 << f;
                LOG_INFO("Feature %s is enabled by default.", desc->name);
            } else {
                LOG_INFO("Feature %s is disabled by default..", desc->name);
            }
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
