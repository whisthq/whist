/**
 * Copyright Fractal Computers, Inc. 2020
 * @file window_name.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

TODO
*/

#if defined(_WIN32)
// TODO(anton) implement functionality for windows servers
void init_window_name_listener() {}
void get_focused_window_name(char* name) {}
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#include "../core/fractal.h"
#include "window_name.h"

static SDL_Thread* window_name_listener_thread;
static SDL_mutex* window_name_mutex;
static char window_name[WINDOW_NAME_MAXLEN];  // protected by window_name_mutex

int window_name_listener(void* opaque);

void init_window_name_listener() {
    // TODO(anton) do nothing if already initialized
    window_name_mutex = SDL_CreateMutex();
    window_name_listener_thread =
        SDL_CreateThread(window_name_listener, "window_name_listener", NULL);
}

void get_focused_window_name(char* name) {
    /*
     * Get the name of the focused window.
     *
     * Arguments:
     *     name (char*): pointer to location to write name
     */
    SDL_LockMutex(window_name_mutex);
    strncpy(name, window_name, WINDOW_NAME_MAXLEN);
    SDL_UnlockMutex(window_name_mutex);
}

// https://gist.github.com/kui/2622504
void get_window_name(Display* d, Window w, char* name) {
    XTextProperty prop;
    Status s;

    s = XGetWMName(d, w, &prop);  // see man
    if (s) {
        int count = 0, result;
        char** list = NULL;
        result = XmbTextPropertyToTextList(d, &prop, &list, &count);  // see man
        if (result == Success) {
            LOG_INFO("window name: %s\n", list[0]);
            strncpy(name, list[0], WINDOW_NAME_MAXLEN);
            name[WINDOW_NAME_MAXLEN - 1] = '\0';
            XFreeStringList(list);
        } else {
            LOG_ERROR("window name: XmbTextPropertyToTextList\n");
        }
    } else {
        LOG_ERROR("window name: XGetWMName\n");
    }
}

int window_name_listener(void* opaque) {
    // TODO(anton) listen for event
    Display* display = XOpenDisplay(NULL);
    Window focus;
    int revert;
    while (true) {
        XGetInputFocus(display, &focus, &revert);
        char name[WINDOW_NAME_MAXLEN];
        get_window_name(display, focus, name);
        SDL_LockMutex(window_name_mutex);
        if (strcmp(name, window_name) != 0) {
            strncpy(window_name, name, WINDOW_NAME_MAXLEN);
        }
        SDL_UnlockMutex(window_name_mutex);
        SDL_Delay(100);
    }
    return 0;
}

#endif
