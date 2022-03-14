/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sdlscreeninfo.c
 * @brief This file contains SDL screen info code, used to retrive information
 *        about SDL Windows.
============================
Usage
============================

Use "physical screen" functions to retrieve real number of pixels (say,
2880x1800), and "virtual screen" functions to retrieve non-DPI resolution (say,
1440x900)
*/

#include "sdlscreeninfo.h"

int get_window_pixel_width(SDL_Window *window) {
    int w;
    SDL_GL_GetDrawableSize(window, &w, NULL);
    return w;
}

int get_window_pixel_height(SDL_Window *window) {
    int h;
    SDL_GL_GetDrawableSize(window, NULL, &h);
    return h;
}

int get_window_virtual_width(SDL_Window *window) {
    int w;
    SDL_GetWindowSize(window, &w, NULL);
    return w;
}

int get_window_virtual_height(SDL_Window *window) {
    int h;
    SDL_GetWindowSize(window, NULL, &h);
    return h;
}
