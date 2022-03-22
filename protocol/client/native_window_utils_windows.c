/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file native_window_utils_windows.c
 * @brief This file implements Windows APIs for native window
 *        modifications not handled by SDL.
============================
Usage
============================

Use these functions to modify the appearance or behavior of the native window
underlying the SDL window. These functions will almost always need to be called
from the same thread as SDL window creation and SDL event handling; usually this
is the main thread.
*/

/*
============================
Includes
============================
*/

// Easy way to import all of our Windows dependencies
// Note that this must be imported first for SDL to link properly.
#include <whist/core/whist.h>

#include <SDL2/SDL_syswm.h>
#include "native_window_utils.h"

HWND get_native_window(SDL_Window* window) {
    SDL_SysWMinfo sys_info = {0};
    SDL_GetWindowWMInfo(window, &sys_info);
    return sys_info.info.win.window;
}

void hide_native_window_taskbar(void) { LOG_INFO("Not implemented on Windows."); }

void init_native_window_options(SDL_Window* window) { return; }

int set_native_window_color(SDL_Window* window, WhistRGBColor color) {
    LOG_INFO("Not implemented on Windows.");
    return 0;
}

int get_native_window_dpi(SDL_Window* window) {
    /*
        Get the DPI for the display of the provided window.

        Arguments:
            window (SDL_Window*):       SDL window whose DPI to retrieve.

        Returns:
            (int): The DPI as an int, with 96 as a base.
    */

    HWND native_window = get_native_window(window);
    return (int)GetDpiForWindow(native_window);
}

void declare_user_activity(void) {
    // TODO: Implement actual wake-up code
    // For now, we'll just disable the screensaver
    SDL_DisableScreenSaver();
}

void initialize_out_of_window_drag_handlers(void) { LOG_INFO("Not implemented on Windows."); }

void destroy_out_of_window_drag_handlers(void) { LOG_INFO("Not implemented on Windows."); }
