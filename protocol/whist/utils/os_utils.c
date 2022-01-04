/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file os_utils.c
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "os_utils.h"
#include <whist/core/whist.h>
#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

/*
============================
Defines
============================
*/

#define WHIST_KB_DEFAULT_LAYOUT "us"

#ifdef __APPLE__
// If this array gets any longer than 1000 or so,
// we should probably swap to a C++ std::map<string, string>
const char* apple_keyboard_mappings[][2] = {
    {"com.apple.keylayout.USExtended", "us"},  {"com.apple.keylayout.US", "us"},
    {"com.apple.keylayout.Italian-Pro", "it"}, {"com.apple.keylayout.Italian", "it"},
    {"com.apple.keylayout.Arabic", "ara"},     {"com.apple.keylayout.ABC-QWERTZ", "de"},
    {"com.apple.keylayout.German", "de"},      {"com.apple.keylayout.Canadian-CSA", "ca"},
    {"com.apple.keylayout.ABC-AZERTY", "fr"},  {"com.apple.keylayout.French", "fr"},
    {"com.apple.keylayout.SwissFrench", "fr"}, {"com.apple.keylayout.LatinAmerican", "latam"},
    {"com.apple.keylayout.Spanish", "es"},     {"com.apple.keylayout.Hebrew", "il"},
};

#define NUM_APPLE_KEYBOARD_MAPPINGS \
    ((int)sizeof(apple_keyboard_mappings) / (int)sizeof(apple_keyboard_mappings[0]))
#endif

WhistKeyboardLayout get_keyboard_layout() {
    WhistKeyboardLayout whist_layout = {0};
    // Set the default layout, in-case the code below fails to set the keyboard layout
    safe_strncpy(whist_layout.layout_name, WHIST_KB_DEFAULT_LAYOUT,
                 sizeof(whist_layout.layout_name));

#ifdef __APPLE__
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    // get input source id - kTISPropertyInputSourceID
    // get layout name - kTISPropertyLocalizedName
    CFStringRef layout_id = TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
    char layout[128] = {0};
    Boolean res = CFStringGetCString(layout_id, layout, sizeof(layout), kCFStringEncodingUTF8);
    if (!res) {
        LOG_ERROR("CFStringGetCString failed!");
        return whist_layout;
    }

    bool found = false;
    for (int i = 0; i < NUM_APPLE_KEYBOARD_MAPPINGS; i++) {
        if (strncmp(apple_keyboard_mappings[i][0], layout, sizeof(layout)) == 0) {
            // Copy new name
            safe_strncpy(whist_layout.layout_name, apple_keyboard_mappings[i][1],
                         sizeof(whist_layout.layout_name));
            found = true;
            break;
        }
    }

    if (!found) {
        // Log the unrecognized keyboard layout,
        // so we can add support for it if we see usage of it
        LOG_ERROR("Mac Keyboard Layout was not recognized! %s", layout);
    }
#else
    // Unimplemented on Windows/Linux, so always assuming US layout for now
#endif

    // Return the whist layout
    return whist_layout;
}

void set_keyboard_layout(WhistKeyboardLayout requested_layout) {
    static char current_layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH] = WHIST_KB_DEFAULT_LAYOUT;

    if (requested_layout.layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH - 1] != '\0') {
        LOG_ERROR("Could not set layout name! last character was not NULL!");
        return;
    }

    // Don't set the keyboard if nothing changed
    if (memcmp(current_layout_name, requested_layout.layout_name, sizeof(current_layout_name)) ==
        0) {
        return;
    }

    // Otherwise, copy into current_layout_name and handle the new current_layout_name
    memcpy(current_layout_name, requested_layout.layout_name, sizeof(current_layout_name));

#ifdef __linux__
    char cmd_buf[1024];
    int bytes_written =
        snprintf(cmd_buf, sizeof(cmd_buf), "setxkbmap -layout %s", current_layout_name);

    if (bytes_written >= 0) {
        runcmd(cmd_buf, NULL);
    }
#else
    LOG_FATAL("Unimplemented on Mac/Windows!");
#endif
}
