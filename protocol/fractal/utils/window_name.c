/**
 * Copyright Fractal Computers, Inc. 2020
 * @file window_name.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

init_window_name_listener();

char name[WINDOW_NAME_MAXLEN];
get_focused_window_name(name);
*/

#if defined(_WIN32)
// TODO(anton) implement functionality for windows servers
void init_window_name_listener() {}
void destroy_window_name_listener() {}
void get_focused_window_name(char* name_return) {}
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#include "../core/fractal.h"
#include "window_name.h"

static SDL_mutex* window_name_mutex;
static char window_name[WINDOW_NAME_MAXLEN];  // protected by window_name_mutex
static SDL_Thread* window_name_listener_thread;
static bool connected;

int window_name_listener(void* opaque);

void init_window_name_listener() {
    if (!connected) {
        // do nothing if already initialized
        connected = true;
        window_name_mutex = SDL_CreateMutex();
        window_name_listener_thread =
            SDL_CreateThread(window_name_listener, "window_name_listener", NULL);
    }
}

void destroy_window_name_listener() {
    connected = false;
    SDL_WaitThread(window_name_listener_thread, NULL);
    SDL_DestroyMutex(window_name_mutex);
}

void get_focused_window_name(char* name_return) {
    /*
     * Get the name of the focused window.
     *
     * Arguments:
     *     name (char*): pointer to location to write name
     */
    SDL_LockMutex(window_name_mutex);
    strncpy(name_return, window_name, WINDOW_NAME_MAXLEN);
    SDL_UnlockMutex(window_name_mutex);
}

// https://gist.github.com/kui/2622504
void get_window_name(Display* d, Window w, char* name_return) {
    XTextProperty prop;
    Status s;

    s = XGetWMName(d, w, &prop);  // see man
    if (s) {
        int count = 0, result;
        char** list = NULL;
        result = XmbTextPropertyToTextList(d, &prop, &list, &count);  // see man
        if (result == Success) {
            LOG_INFO("window name: %s\n", list[0]);
            strncpy(name_return, list[0], WINDOW_NAME_MAXLEN);
            name_return[WINDOW_NAME_MAXLEN - 1] = '\0';
            XFreeStringList(list);
        } else {
            LOG_ERROR("window name: XmbTextPropertyToTextList\n");
        }
    } else {
        LOG_ERROR("window name: XGetWMName\n");
    }
}

int window_name_listener(void* opaque) {
    Display* display = XOpenDisplay(NULL);
    Window focus;
    int revert;
    XGetInputFocus(display, &focus, &revert);
    XSelectInput(display, focus, PropertyChangeMask);  // listen for property changes
    XEvent event;
    Atom wm_name = XInternAtom(display, "_NET_WM_NAME", false);
    while (connected) {
        XNextEvent(display, &event);
        if (event.type == PropertyNotify && event.xproperty.atom == wm_name) {
            SDL_LockMutex(window_name_mutex);
            get_window_name(display, focus, window_name);
            SDL_UnlockMutex(window_name_mutex);
        }
        SDL_Delay(10);
    }
    XCloseDisplay(display);
    return 0;
}

#endif
