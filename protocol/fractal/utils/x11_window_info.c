/**
 * Copyright Fractal Computers, Inc. 2021
 * @file x11_window_info.h
 * @brief This file contains all the code for getting X11-specific window information.
============================
Usage
============================

init_x11_window_info_getter();

char name[WINDOW_NAME_MAXLEN + 1];
get_focused_window_name(name);

destroy_x11_window_info_getter();
*/

#include <fractal/core/fractal.h>
#include "x11_window_info.h"
#include <locale.h>

#if defined(_WIN32)

// TODO: implement functionality for windows servers
void init_x11_window_info_getter() {
    LOG_ERROR("UNIMPLEMENTED: init_x11_window_info_getter on Win32");
}
int get_focused_window_name(char* name_return) {
    LOG_ERROR("UNIMPLEMENTED: get_focused_window_name on Win32");
    return -1;
}
bool is_focused_window_fullscreen() {
    LOG_ERROR("UNIMPLEMENTED: is_focused_window_fullscreen on Win32");
    return false;
}
void destroy_x11_window_info_getter() {
    LOG_ERROR("UNIMPLEMENTED: destroy_x11_window_info_getter on Win32");
}

#elif __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

Display* display;

void init_x11_window_info_getter() {
    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }
}

Window get_focused_window() {
    Window w;
    // TODO: Is this needed, or can we pass in NULL?
    int revert;
    XGetInputFocus(display, &w, &revert);
    return w;
}

/**
 * @brief                          Query whether the focused window is fullscreen or not.
 *
 * @returns                        0 if not fullscreen, 1 if fullscreen.
 *
 * @note                           By "fullscreen", we mean that the window is rendering directly
 *                                 to the screen, not through the window manager. Examples include
 *                                 fullscreen video playback in a browser, or fullscreen games.
 */
bool is_focused_window_fullscreen() {
    /*
     * Query whether the focused window is fullscreen or not.
     *
     *  Return:
     *      (bool): 0 if not fullscreen, 1 if fullscreen.
     *
     *  Note:
     *      By "fullscreen", we mean that the window is rendering directly
     *      to the screen, not through the window manager. Examples include
     *      fullscreen video playback in a browser, or fullscreen games.
     */
    Window w = get_focused_window();

    if (!w) {
        // No window is active.
        return false;
    }

    Atom state = XInternAtom(display, "_NET_WM_STATE", True);
    Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", True);
    Atom actual_type;
    int format;
    unsigned long num_items, bytes_after;
    uint8_t* data = NULL;
    int res = XGetWindowProperty(display, w, state, 0, ~0L, False, AnyPropertyType, &actual_type,
                                 &format, &num_items, &bytes_after, &data);
    if (res == Success && data) {
        Atom windowprop = ((Atom*)data)[0];
        if (windowprop == fullscreen) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

int get_focused_window_name(char* name_return) {
    /*
     * Get the name of the focused window.
     *
     * Arguments:
     *     name_return (char*): Pointer to location to write name. Must have at least
     *                          WINDOW_NAME_MAXLEN + 1 bytes available.
     *
     *  Return:
     *      ret (int): 0 on success, other integer on failure
     */

    Window w = get_focused_window();

    if (!w) {
        // No window is active.
        return 1;
    }

    // Ask all Fractal & library functions to use the locale defined by the environment. This
    // prevents encoding problems (for example, when it comes to encoding strings in UTF8 format).
    setlocale(LC_ALL, "");

    // https://gist.github.com/kui/2622504
    XTextProperty prop;
    Status s;

    s = XGetWMName(display, w, &prop);
    if (s) {
        int count = 0, result;
        char** list = NULL;
        result = XmbTextPropertyToTextList(display, &prop, &list, &count);
        if (!count) {
            // no window title found
            return 1;
        }
        if (result == Success) {
            safe_strncpy(name_return, list[0], WINDOW_NAME_MAXLEN + 1);
            XFreeStringList(list);
            return 0;
        } else {
            LOG_ERROR("XmbTextPropertyToTextList failed to convert window name to string");
        }
    } else {
        LOG_ERROR("XGetWMName failed to get the name of the focused window");
    }
    return 1;
}

void destroy_x11_window_info_getter() {
    if (display != NULL) {
        XCloseDisplay(display);
        display = NULL;
    }
}

#endif  // __linux__
