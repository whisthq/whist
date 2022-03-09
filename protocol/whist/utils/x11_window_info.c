/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file x11_window_info.c
 * @brief This file contains all the code for getting X11-specific window information.
 */

#include <whist/core/whist.h>
#include "window_info.h"
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

static Display* display;
static char last_window_name[WINDOW_NAME_MAXLEN + 1];
static bool last_window_name_valid;

void init_window_info_getter(void) {
    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }
    last_window_name_valid = false;
}

static Window get_focused_window(void) {
    Window w;
    // TODO: Is this needed, or can we pass in NULL?
    int revert;
    XGetInputFocus(display, &w, &revert);
    return w;
}

bool get_focused_window_name(char** name_return) {
    /*
     * Get the name of the focused window.
     *
     * Arguments:
     *     name_return (char**): Location to write char* of window title name.
     *                           NULL on failure.
     *
     *  Return:
     *      ret (int): true on new window name, false if the same window name or failure
     */

    Window w = get_focused_window();

    *name_return = NULL;

    if (!w) {
        // No window is active.
        return false;
    }

    // Ask all Whist & library functions to use the locale defined by the environment. This
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
            return false;
        }
        if (result == Success) {
            static char cur_window_name[WINDOW_NAME_MAXLEN + 1];
            safe_strncpy(cur_window_name, list[0], WINDOW_NAME_MAXLEN + 1);
            // trim down any dangling utf8 multi-byte characters
            trim_utf8_string(cur_window_name);
            XFreeStringList(list);
            bool same_string =
                !last_window_name_valid || strcmp(last_window_name, cur_window_name) == 0;
            safe_strncpy(last_window_name, cur_window_name, WINDOW_NAME_MAXLEN + 1);
            last_window_name_valid = true;
            *name_return = cur_window_name;
            return !same_string;
        } else {
            LOG_ERROR("XmbTextPropertyToTextList failed to convert window name to string");
        }
    } else {
        // If XGetWName returns 0, it's because the window has no name. In Chrome, this is the case
        // with pop-up windows, which is why we mark this as LOG_INFO rather than a LOG_ERROR.
        LOG_INFO("Focused window %lu has no name", w);
    }
    return false;
}

bool is_focused_window_fullscreen(void) {
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

void destroy_window_info_getter(void) {
    if (display != NULL) {
        XCloseDisplay(display);
        display = NULL;
    }
}
