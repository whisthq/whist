/**
 * Copyright Fractal Computers, Inc. 2020
 * @file window_name.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

init_window_name_getter();

char name[WINDOW_NAME_MAXLEN + 1];
get_focused_window_name(name);

destroy_window_name_getter();
*/

#include <fractal/core/fractal.h>
#include "window_name.h"

#if defined(_WIN32)

// TODO: implement functionality for windows servers
void init_window_name_getter() { LOG_ERROR("UNIMPLEMENTED: init_window_name_getter on Win32"); }
int get_focused_window_name(char* name_return) {
    LOG_ERROR("UNIMPLEMENTED: get_focused_window_name on Win32");
    return -1;
}
void destroy_window_name_getter() {
    LOG_ERROR("UNIMPLEMENTED: destroy_window_name_getter on Win32");
}

#elif __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

Display* display;

void init_window_name_getter() {
    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }
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

    Window w;
    int revert;
    XGetInputFocus(display, &w, &revert);

    if (!w) {
        // No window is active.
        return 1;
    }

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
            // TODO(anton): actually address non-ASCII encodings rather than just replacing
            // non-ASCII characters with spaces
            for (int i = 0; list[0][i] != 0; i++) {
                if ((unsigned char)list[0][i] > 127) {  // > 127 means not ASCII
                    list[0][i] = ' ';
                }
            }
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

void destroy_window_name_getter() {
    if (display != NULL) {
        XCloseDisplay(display);
        display = NULL;
    }
}

#endif  // __linux__
