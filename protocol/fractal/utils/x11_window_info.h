#ifndef WINDOW_NAME_H
#define WINDOW_NAME_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file x11_window_info.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

init_x11_window_info_getter();

char name[WINDOW_NAME_MAXLEN + 1];
get_focused_window_name(name);

destroy_x11_window_info_getter();
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>

/*
============================
Defines
============================
*/

// MAXLENs are the max length of the string they represent, _without_ the null character.
// Therefore, whenever arrays are created or length of the string is compared, we should be
// comparing to *MAXLEN + 1
#define WINDOW_NAME_MAXLEN 127

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize variables required to get window names.
 *
 */
void init_x11_window_info_getter();

/**
 * @brief                          Get the name of the focused window.
 *
 * @param name_return              Address to write name. Should have at least WINDOW_NAME_MAXLEN +
 *                                 1 bytes available.
 *
 * @returns                        0 on success, any other int on failure.
 */
int get_focused_window_name(char* name_return);

/**
 * @brief                          Query whether the focused window is fullscreen or not.
 *
 * @returns                        0 if not fullscreen, 1 if fullscreen.
 *
 * @note                           By "fullscreen", we mean that the window is rendering directly
 *                                 to the screen, not through the window manager. Examples include
 *                                 fullscreen video playback in a browser, or fullscreen games.
 */
bool is_focused_window_fullscreen();

/**
 * @brief                          Destroy variables that were initialized.
 *
 */
void destroy_x11_window_info_getter();

#endif  // WINDOW_NAME_H
