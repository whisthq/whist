/**
 * Copyright Fractal Computers, Inc. 2020
 * @file window_name.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

init_window_name_getter();

char name[WINDOW_NAME_MAXLEN];
get_focused_window_name(name);

destroy_window_name_getter();
*/

#if defined(_WIN32)
// TODO(anton) implement functionality for windows servers
void init_window_name_getter() {}
int get_focused_window_name(char* name_return) { return -1; }
void destroy_window_name_getter() {}
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#include "string_utils.h"
#include "../core/fractal.h"
#include "window_name.h"

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
     *                          WINDOW_NAME_MAXLEN bytes available.
     *
     *  Return:
     *      ret (int): 0 on success, other integer on failure
     */

    Window w;
    int revert;
    XGetInputFocus(display, &w, &revert);

    // https://gist.github.com/kui/2622504
    XTextProperty prop;
    Status s;

    s = XGetWMName(display, w, &prop);
    if (s) {
        int count = 0, result;
        char** list = NULL;
        result = XmbTextPropertyToTextList(display, &prop, &list, &count);
        if (result == Success) {
            safe_strncpy(name_return, list[0], WINDOW_NAME_MAXLEN);
            XFreeStringList(list);
            return 0;
        } else {
            LOG_ERROR("window name: XmbTextPropertyToTextList\n");
        }
    } else {
        LOG_ERROR("window name: XGetWMName\n");
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
