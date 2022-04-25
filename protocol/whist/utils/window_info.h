#ifndef WINDOW_INFO_H
#define WINDOW_INFO_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file window_info.h
 * @brief This file contains all the code for getting and setting window attributes

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
//
Create a capture device for the display whose windows you want to work with. To get the active
window or a list of windows for the client to display, call get_active_window or get_valid_windows,
respectively; pass the resulting WhistWindows to the getter/setter functions here.

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

typedef struct WhistWindowManager WhistWindowManager;

typedef bool (*get_focused_window_name_cb)(WhistWindowManager* manager, char** name_return);
typedef bool (*is_focused_window_fullscreen_cb)(WhistWindowManager* manager);
typedef void (*get_valid_windows_cb)(WhistWindowManager* manager, LinkedList* window_list);

typedef WhistWindow (*get_active_window_cb)(WhistWindowManager* manager);

/** @brief abstract window lister and manager */
struct WhistWindowManager {
	get_focused_window_name_cb get_focused_window_name;
	is_focused_window_fullscreen_cb is_focused_window_fullscreen;
	get_valid_windows_cb get_valid_windows;

	get_active_window_cb get_active_window;
};

WhistWindowManager* window_manager_get(DesktopType kind, void *args);

/*
============================
Public Functions
============================
*/



/**
 * @brief                          Initialize window info getter
 *
 */
void init_window_info_getter(WhistWindowManager *manager);

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
void destroy_window_info_getter(WhistWindowManager *manager);

/**
 * @brief                          Get a linked list of windows we should stream
 *                                 (e.g. Chrome windows and popups)
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param window_list              Linked list to append the window list into
 *
 */
void get_valid_windows(WhistWindowManager *manager, LinkedList* window_list);

/**
 * @brief                          Fill WhistWindowData* struct with x/y/w/h/etc. of whist_window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             The WhistWindow to fill attributes for
 */
void get_window_attributes(WhistWindowManager *manager, WhistWindow* whist_window);

/**
 * @brief                          Return the active window, either using NET_ACTIVE_WINDOW atom
 * (preferred) or XGetInputFocus
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @returns                        WhistWindow struct representing the active window
 *
 */
WhistWindow get_active_window(WhistWindowManager *manager);
// TODO: replace with a function that doesn't just return the raw string?
// or explicitly make one for Window (private) and one for WhistWindow (public)
// char* get_window_name(CaptureDevice* capture_device, WhistWindow whist_window);
//
/**
 * @brief                          Return whether or not the window is resizable (e.g. popups or
 * right click menus are not resizable, but Chrome is)
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow whose properties we are querying
 *
 * @returns                        whether or not whist_window is resizable
 *
 */
bool is_window_resizable(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Move the window to (x, y) and resize it to (width, height). This
 * only sends a request to the window manager to do so; the window manager can reject the request,
 * e.g. if the window manager is started maximized.
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow we are moving and resizing
 *
 * @param x                        Desired x-coordinate
 *
 * @param y                        Desired y-coordinate
 *
 * @param width                    Desired width
 *
 * @param height                   Desired height
 *
 */
void move_resize_window(WhistWindowManager *manager, WhistWindow whist_window, int x, int y,
                        int width, int height);

/**
 * @brief                          Close the given window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to close
 *
 */
void close_window(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Minimize the given window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to minimize
 *
 */
void minimize_window(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Maximize the given window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to maximize
 *
 */
void maximize_window(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Fullscreen the given window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to fullscreen
 *
 */
void fullscreen_window(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Bring given window to top
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to bring to top
 *
 */
void bring_window_to_top(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Unminimize the given window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to unminimize
 *
 */
void unminimize_window(WhistWindowManager *manager, WhistWindow whist_window);

/**
 * @brief                          Unfullscreen the given window
 *
 * @param capture_device           Capture device whose display we are using
 *
 * @param whist_window             WhistWindow to unfullscreen
 *
 */
void unfullscreen_window(WhistWindowManager *manager, WhistWindow whist_window);

#endif  // WINDOW_INFO_H
