#ifndef NATIVE_WINDOW_UTILS_H
#define NATIVE_WINDOW_UTILS_H

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file native_window_utils.h
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

#include <SDL2/SDL.h>
#include <whist/utils/color.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Hide the taskbar icon for the app. This only works on
 *                                 macOS (for Windows and Linux, SDL already implements this in
 *                                 the `SDL_WINDOW_SKIP_TASKBAR` flag).
 */
void hide_native_window_taskbar(void);

/**
 * @brief                          Initialize the customized native window. This is called from
 *                                 the main thread right after the window finishes loading.
 *
 * @param window                   The SDL window wrapper for the NSWindow to customize.
 */
void init_native_window_options(SDL_Window* window);

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
int set_native_window_color(SDL_Window* window, WhistRGBColor color);

/**
 * @brief                          Get the DPI for the display of the provided window.
 *
 * @param window                   The SDL window handle whose DPI to retrieve.
 *
 * @returns                        The DPI as an int, with 96 as a base.
 */
int get_native_window_dpi(SDL_Window* window);

/**
 * @brief                          Set up callbacks for out of window drag events
 *
 */
void initiate_out_of_window_drag_handlers(void);


/**
 * @brief                          Declares that UserActivity has occured,
 *                                 resetting the timer for screensavers/sleepmode
 */
void declare_user_activity(void);

#endif  // NATIVE_WINDOW_UTILS_H
