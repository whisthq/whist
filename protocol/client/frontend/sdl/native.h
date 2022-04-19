#ifndef WHIST_CLIENT_FRONTEND_SDL_NATIVE_H
#define WHIST_CLIENT_FRONTEND_SDL_NATIVE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file native.h
 * @brief This file exposes platform-independent APIs for
 *        native window modifications not handled by SDL.
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

#include "common.h"

/**
 * @brief                          Hide the taskbar icon for the app. This only works on
 *                                 macOS (for Windows and Linux, SDL already implements this in
 *                                 the `SDL_WINDOW_SKIP_TASKBAR` flag).
 */
void sdl_native_hide_taskbar(void);

/**
 * @brief                          Initialize the customized native window. This is called from
 *                                 the main thread right after the window finishes loading.
 *
 * @param window                   The SDL window wrapper for the NSWindow to customize.
 */
void sdl_native_init_window_options(SDL_Window* window);

/**
 * @brief                          Set the color of the titlebar of the provided window, and
 *                                 the corresponding titlebar text as well.
 *
 * @param window                   The SDL window handle whose titlebar to modify.
 *
 * @param color                    The RGB color to use when setting the titlebar.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int sdl_native_set_titlebar_color(SDL_Window* window, const WhistRGBColor* color);

/**
 * @brief                          Get the DPI for the display of the provided window.
 *
 * @param window                   The SDL window handle whose DPI to retrieve.
 *
 * @returns                        The DPI as an int, with 96 as a base.
 *
 * @note                           Should only be called inside main thread, at least on MacOS.
 */
int sdl_native_get_dpi(SDL_Window* window);

/**
 * @brief                          Declares that user activity has occured,
 *                                 resetting the timer for screensavers and sleep mode.
 */
void sdl_native_declare_user_activity(void);

/**
 * @brief                          Set up callbacks for out of window drag events
 *
 */
void sdl_native_init_external_drag_handler(WhistFrontend* frontend);

/**
 * @brief                          Destroy callbacks for out of window drag events
 *
 */
void sdl_native_destroy_external_drag_handler(WhistFrontend* frontend);

#endif  // WHIST_CLIENT_FRONTEND_SDL_NATIVE_H
