/**
 * Copyright Fractal Computers, Inc. 2020
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

int get_virtual_screen_width() {
    SDL_DisplayMode DM;
    //    int res = SDL_GetCurrentDisplayMode(0, &DM);
    int res = SDL_GetDesktopDisplayMode(0, &DM);
    if (res)
        LOG_WARNING("SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
    return DM.w;
}

int get_virtual_screen_height() {
    SDL_DisplayMode DM;
    //    int res = SDL_GetCurrentDisplayMode(0, &DM);
    int res = SDL_GetDesktopDisplayMode(0, &DM);
    if (res)
        LOG_WARNING("SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
    return DM.h;
}

int get_pixel_screen_width(SDL_Window *window) {
    int w = (int)(1.0 * get_virtual_screen_width() *
                      get_window_pixel_width(window) /
                      get_window_virtual_width(window) +
                  0.5);
    return w;
}

int get_pixel_screen_height(SDL_Window *window) {
    int h = (int)(1.0 * get_virtual_screen_height() *
                      get_window_pixel_height(window) /
                      get_window_virtual_height(window) +
                  0.5);
    return h;
}