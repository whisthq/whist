/**
 * Copyright Fractal Computers, Inc. 2021
 * @file native_window_utils_x11.c
 * @brief This file implements X11 APIs for native window
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

#include "native_window_utils.h"
#include <fractal/logging/logging.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>

void hide_native_window_taskbar() { LOG_INFO("Not implemented on X11."); }

int set_native_window_color(SDL_Window* window, FractalRGBColor color) {
    LOG_INFO("Not implemented on X11.");
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

    // We just fall back to the SDL implementation here.
    int display_index = SDL_GetWindowDisplayIndex(window);
    float dpi;
    SDL_GetDisplayDPI(display_index, NULL, &dpi, NULL);
    return (int)dpi;
}

FractalYUVColor get_frame_color(uint8_t* y_data, uint8_t* u_data, uint8_t* v_data,
                                bool using_hardware) {
    UNUSED(using_hardware);
    FractalYUVColor yuv_color = {0};
    if (y_data && u_data && v_data) {
        yuv_color.y = y_data[0];
        yuv_color.u = u_data[0];
        yuv_color.v = v_data[0];
    }
    return yuv_color;
}
