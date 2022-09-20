/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file windows_window_info.c
 * @brief This file contains all the code for getting Windows-specific window information.
 */

#include <whist/core/whist.h>
#include "window_info.h"
#include <locale.h>

// TODO: implement functionality for windows servers
void init_window_info_getter(void) {
    // We only error on init/destroy, to prevent spam
    LOG_ERROR("UNIMPLEMENTED: init_window_info_getter on Win32");
}

bool is_focused_window_fullscreen(void) { return false; }
void destroy_window_info_getter(void) {
    LOG_ERROR("UNIMPLEMENTED: destroy_x11_window_info_getter on Win32");
}
