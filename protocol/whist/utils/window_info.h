#ifndef WINDOW_INFO_H
#define WINDOW_INFO_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file window_info.h
 * @brief This file contains all the code for getting the name of a window.

============================
Usage
============================

// TODO: integrate the old get_focused_window_name with new code
// we'll use CaptureDevice instead of a global window_info_getter

init_window_info_getter();

char name[WINDOW_NAME_MAXLEN + 1];
get_focused_window_name(name);

destroy_window_info_getter();

// Note that this library is not thread-safe,
// there must only be one window_info_getter live at any one time

*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include <whist/video/capture/capture.h>

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
 * @brief                          Initialize window info getter
 *
 */
void init_window_info_getter(void);

/**
 * @brief                          Get the name of the focused window.
 *
 * @param name_return              Address to write window title name to.
 *
 * @returns                        true if the window name is new,
 *                                 false if the window name is the same or on failure
 */
bool get_focused_window_name(char** name_return);

/**
 * @brief                          Query whether the focused window is fullscreen or not.
 *
 * @returns                        false if not fullscreen, true if fullscreen.
 *
 * @note                           By "fullscreen", we mean that the window is rendering directly
 *                                 to the screen, not through the window manager. Examples include
 *                                 fullscreen video playback in a browser, or fullscreen games.
 */
bool is_focused_window_fullscreen(void);

/**
 * @brief                          Destroy window info getter
 *
 */
void destroy_window_info_getter(void);

// Added functions for multiwindow
void get_valid_windows(CaptureDevice* capture_device, LinkedList* list);
void get_window_attributes(CaptureDevice* capture_device, WhistWindow whist_window, WhistWindowData* d);
WhistWindow get_active_window(CaptureDevice* capture_device);
// TODO: replace with a function that doesn't just return the raw string?
// or explicitly make one for Window (private) and one for WhistWindow (publi)
// char* get_window_name(CaptureDevice* capture_device, WhistWindow whist_window);
bool is_window_resizable(CaptureDevice* capture_device, WhistWindow whist_window);
void move_resize_window(CaptureDevice* device, WhistWindow whist_window, int x, int y, int width, int height);
void close_window(CaptureDevice* device, WhistWindow whist_window);
void minimize_window(CaptureDevice* capture_device, WhistWindow whist_window);
void maximize_window(CaptureDevice* capture_device, WhistWindow whist_window);
void fullscreen_window(CaptureDevice* capture_device, WhistWindow whist_window);
void bring_window_to_top(CaptureDevice* capture_device, WhistWindow whist_window);
void unminimize_window(CaptureDevice* capture_device, WhistWindow whist_window);
void unfullscreen_window(CaptureDevice* capture_device, WhistWindow whist_window);

#endif  // WINDOW_INFO_H
