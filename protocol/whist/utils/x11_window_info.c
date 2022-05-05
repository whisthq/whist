/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file x11_window_info.c
 * @brief This file contains all the code for getting X11-specific window information.
 */

#include <whist/core/whist.h>
#include "window_info.h"
#include <locale.h>
#include <whist/video/capture/x11capture.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

#define INIT_ATOM(DEVICE, ATOM_VAR, NAME)                    \
    static Atom ATOM_VAR = 0;                                \
    if (!ATOM_VAR) {                                         \
        ATOM_VAR = XInternAtom(DEVICE->display, NAME, True); \
    }

static Display* display;
static char last_window_name[WINDOW_NAME_MAXLEN + 1];
static bool last_window_name_valid;

// Privates

bool x11_get_window_property(X11CaptureDevice* device, Window w, Atom property, Atom req_type,
                             unsigned long* nitems_return, unsigned char** prop_return);
void send_message_to_root(X11CaptureDevice* device, XEvent* event_send);
XEvent create_change_state_message(X11CaptureDevice* device, Window w, long action, Atom property1,
                                   Atom property2);
char* get_window_name(X11CaptureDevice* device, Window w);
void get_valid_windows_helper(X11CaptureDevice* device, LinkedList* list, Window curr);

void init_window_info_getter(void) {
    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }
    last_window_name_valid = false;
}

void get_valid_windows_helper(X11CaptureDevice* device, LinkedList* list, Window curr) {
    Window parent;
    Window* children;
    unsigned int nchildren;
    XQueryTree(device->display, curr, &device->root, &parent, &children, &nchildren);
    // check the dimensions of each window
    XWindowAttributes attr;
    XGetWindowAttributes(device->display, curr, &attr);
    if (attr.x >= 0 && attr.y >= 0 && attr.width >= MIN_SCREEN_WIDTH &&
        attr.height >= MIN_SCREEN_HEIGHT && *get_window_name(device, curr) != '\0') {
        LOG_INFO("Valid window %s has %d children, position %d, %d, dimensions %d x %d",
                 get_window_name(device, curr), nchildren, attr.x, attr.y, attr.width, attr.height);
        WhistWindow* valid_window = safe_malloc(sizeof(WhistWindow));
        valid_window->window = curr;
        linked_list_add_tail(list, valid_window);
    }

    if (nchildren != 0) {
        for (unsigned int i = 0; i < nchildren; i++) {
            get_valid_windows_helper(device, list, children[i]);
        }
    }
}

void get_valid_windows(CaptureDevice* capture_device, LinkedList* list) {
    X11CaptureDevice* device = capture_device->x11_capture_device;
    get_valid_windows_helper(device, list, device->root);
}

void get_window_attributes(CaptureDevice* capture_device, WhistWindow whist_window,
                           WhistWindowData* d) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    XWindowAttributes attr;
    XGetWindowAttributes(device->display, w, &attr);
    d->width = attr.width;
    d->height = attr.height;
    d->x = attr.x;
    d->y = attr.y;
}

// HELPER
// wrapper around XGetWindowProperty, returns whether successful or not
bool x11_get_window_property(X11CaptureDevice* device, Window w, Atom property, Atom req_type,
                             unsigned long* nitems_return, unsigned char** prop_return) {
    static Atom actual_type;
    static int actual_format;
    static unsigned long bytes_after;
    return (XGetWindowProperty(
                device->display, w, property, 0,
                65536,  // the internet doesn't seem to agree on what long_length should be
                False, req_type, &actual_type, &actual_format, nitems_return, &bytes_after,
                prop_return) == Success);
}

WhistWindow get_active_window(CaptureDevice* capture_device) {
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_active_window, "_NET_ACTIVE_WINDOW");
    WhistWindow w;
    static unsigned long nitems;
    static unsigned char* result;  // window stored here

    if (x11_get_window_property(device, device->root, net_active_window, AnyPropertyType, &nitems,
                                &result) &&
        *(unsigned long*)result != 0) {
        Window active_window = (Window) * (unsigned long*)result;
        // XFree(result);
        w.window = active_window;
    } else {
        // revert to XGetInputFocus
        Window focus;
        int revert;
        XGetInputFocus(device->display, &focus, &revert);
        if (focus != PointerRoot) {
            w.window = focus;
        } else {
            LOG_INFO("No active window found, setting root as active");
            w.window = device->root;
        }
    }
    return w;
}

// utf-8 string; helper (processed by something like get_focused_window_name with the
// last_valid_title and everything)
char* get_window_name(X11CaptureDevice* device, Window w) {
    // first try using EWMH
    INIT_ATOM(device, net_wm_name, "_NET_WM_NAME");
    INIT_ATOM(device, utf8_string, "UTF8_STRING");
    static unsigned long nitems;
    static unsigned char* name;  // name stored here

    if (x11_get_window_property(device, w, net_wm_name, utf8_string, &nitems, &name)) {
        return (char*)name;
    } else {
        // fall back to XGetWMName
        XTextProperty prop;
        // unclear if needed
        XLockDisplay(device->display);
        Status s = XGetWMName(device->display, w, &prop);
        if (s) {
            int count = 0;
            char** list = NULL;
            int result = XmbTextPropertyToTextList(device->display, &prop, &list, &count);
            XUnlockDisplay(device->display);
            if (count && result == Success) {
                return list[0];
            } else {
                return "";
            }
        } else {
            XUnlockDisplay(device->display);
            return "";
        }
    }
}

bool is_window_resizable(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_allowed_actions, "_NET_WM_ALLOWED_ACTIONS");
    INIT_ATOM(device, atom_array, "ATOM[]");
    INIT_ATOM(device, net_wm_action_resize, "_NET_WM_ACTION_RESIZE");
    static unsigned long nitems;
    static unsigned char* actions;  // name stored here

    if (x11_get_window_property(device, w, net_wm_allowed_actions, atom_array, &nitems, &actions)) {
        Atom* allowed_actions = (Atom*)actions;
        // check if one of the actions is net_wm_action_resize
        for (int i = 0; i < (int)nitems; i++) {
            if (allowed_actions[i] == net_wm_action_resize) {
                return true;
            }
        }
        return false;
    }
    LOG_ERROR("Couldn't get allowed actions, assuming window is resizable!");
    return true;
}

// HELPER
void send_message_to_root(X11CaptureDevice* device, XEvent* event_send) {
    XSendEvent(device->display, device->root, False,
               SubstructureNotifyMask | SubstructureRedirectMask, event_send);
}

void move_resize_window(CaptureDevice* capture_device, WhistWindow whist_window, int x, int y,
                        int width, int height) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_moveresize_window, "_NET_MOVERESIZE_WINDOW");
    static long gravity_flags = 15 << 8;  // sets gravity to 0, x/y/w/h to 1, source to 0

    XEvent xevent = {0};
    XClientMessageEvent xclient = {0};
    xclient.type = ClientMessage;
    xclient.window = w;
    xclient.display = device->display;
    xclient.message_type = net_moveresize_window;
    xclient.format = 32;
    xclient.data.l[0] = gravity_flags;
    xclient.data.l[1] = x;
    xclient.data.l[2] = y;
    xclient.data.l[3] = width;
    xclient.data.l[4] = height;
    xevent.xclient = xclient;
    send_message_to_root(device, &xevent);
}

void close_window(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_close_window, "_NET_CLOSE_WINDOW");

    XEvent xevent = {0};
    XClientMessageEvent xclient = {0};
    xclient.type = ClientMessage;
    xclient.window = w;
    xclient.display = device->display;
    xclient.message_type = net_close_window;
    xclient.format = 32;
    xclient.data.l[0] = 0;  // timestamp: not sure if it should be 0
    xclient.data.l[1] = 0;  // source: 0 for now
    xclient.data.l[2] = 0;
    xclient.data.l[3] = 0;
    xclient.data.l[4] = 0;
    xevent.xclient = xclient;
    send_message_to_root(device, &xevent);
}

// HELPER
XEvent create_change_state_message(X11CaptureDevice* device, Window w, long action, Atom property1,
                                   Atom property2) {
    INIT_ATOM(device, net_wm_state, "_NET_WM_STATE");

    XEvent xevent = {0};
    XClientMessageEvent xclient = {0};
    xclient.type = ClientMessage;
    xclient.window = w;
    xclient.display = device->display;
    xclient.message_type = net_wm_state;
    xclient.format = 32;
    xclient.data.l[0] = action;           // timestamp: not sure if it should be 0
    xclient.data.l[1] = (long)property1;  // source: 0 for now
    xclient.data.l[2] = (long)property2;
    xclient.data.l[3] = 0;
    xclient.data.l[4] = 0;
    xevent.xclient = xclient;
    return xevent;
}

void minimize_window(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_state_hidden, "_NET_WM_STATE_HIDDEN");

    XEvent xevent =
        create_change_state_message(device, w, _NET_WM_STATE_ADD, net_wm_state_hidden, 0);
    send_message_to_root(device, &xevent);
}

void unminimize_window(CaptureDevice* capture_device, WhistWindow whist_window) {
    // I'm not sure what will happen if you call this on a window that's already not minimized --
    // hopefully a noop
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_state_hidden, "_NET_WM_STATE_HIDDEN");

    XEvent xevent =
        create_change_state_message(device, w, _NET_WM_STATE_REMOVE, net_wm_state_hidden, 0);
    send_message_to_root(device, &xevent);
}

void maximize_window(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_state_maximized_vert, "_NET_WM_STATE_MAXIMIZED_VERT");
    INIT_ATOM(device, net_wm_state_maximized_horz, "_NET_WM_STATE_MAXIMIZED_HORZ");

    XEvent xevent = create_change_state_message(
        device, w, _NET_WM_STATE_ADD, net_wm_state_maximized_vert, net_wm_state_maximized_horz);
    send_message_to_root(device, &xevent);
}

void fullscreen_window(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_state_fullscreen, "_NET_WM_STATE_FULLSCREEN");

    XEvent xevent =
        create_change_state_message(device, w, _NET_WM_STATE_ADD, net_wm_state_fullscreen, 0);
    send_message_to_root(device, &xevent);
}

void unfullscreen_window(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_state_fullscreen, "_NET_WM_STATE_FULLSCREEN");

    XEvent xevent =
        create_change_state_message(device, w, _NET_WM_STATE_REMOVE, net_wm_state_fullscreen, 0);
    send_message_to_root(device, &xevent);
}

void bring_window_to_top(CaptureDevice* capture_device, WhistWindow whist_window) {
    Window w = whist_window.window;
    X11CaptureDevice* device = capture_device->x11_capture_device;
    INIT_ATOM(device, net_wm_state_above, "_NET_WM_STATE_ABOVE");

    XEvent xevent =
        create_change_state_message(device, w, _NET_WM_STATE_ADD, net_wm_state_above, 0);
    send_message_to_root(device, &xevent);
}

// superseded by x11_get_active_window
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
    // prevents encoding problems (for example, when it comes to encoding strings in UTF8
    // format).
    setlocale(LC_ALL, "");

    // https://gist.github.com/kui/2622504
    XTextProperty prop;
    Status s;

    XLockDisplay(display);
    s = XGetWMName(display, w, &prop);
    if (s) {
        int count = 0, result;
        char** list = NULL;
        result = XmbTextPropertyToTextList(display, &prop, &list, &count);
        XUnlockDisplay(display);
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
        XUnlockDisplay(display);
        // If XGetWName returns 0, it's because the window has no name. In Chrome, this is the
        // case with pop-up windows, which is why we mark this as LOG_INFO rather than a
        // LOG_ERROR.
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
