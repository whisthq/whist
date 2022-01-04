/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file os_utils.c
============================
Usage
============================
*/

#include "os_utils.h"

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#ifdef _WIN32
#elif __APPLE__
#else
#endif

#define WHIST_KB_DEFAULT_LAYOUT "us"

WhistKeyboardLayout get_keyboard_layout() {
    WhistKeyboardLayout ret = {0};
    safe_strncpy(ret.layout_name, WHIST_KB_DEFAULT_LAYOUT, sizeof(ret.layout_name));
    return ret;
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
