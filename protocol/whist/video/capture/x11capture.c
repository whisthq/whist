/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file x11capture.h
 * @brief This file contains the code to do screen capture via the X11 API on Linux Ubuntu.
============================
Usage
============================

X11CaptureDevice contains all the information used to interface with the X11 screen
capture API and the data of a frame. Call create_x11_capture_device to initialize a device,
x11_capture_screen to capture the screen with said device, and destroy_x11_capture_device when done
capturing frames.
*/

/*
============================
Includes
============================
*/
#include "capture.h"

#include <X11/extensions/Xdamage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <X11/Xutil.h>

/*
============================
Private Functions
============================
*/
int handler(Display* d, XErrorEvent* a);
char* get_window_name(Display* d, Window w);
void log_tree(X11CaptureDevice* device, Window w);

/*
============================
Private Function Implementations
============================
*/
int handler(Display* d, XErrorEvent* a) {
    /*
        X11 error handler allowing us to integrate x11 errors with our error logging system.

        Arguments:
            d (Display*): Unused, needed for the right function signature
            a (XerrorEvent*): error to log

        Returns:
            (int): always 0
    */

    LOG_ERROR("X11 Error: %d", a->error_code);
    return 0;
}

char* get_window_name(Display* d, Window w) {
    XTextProperty prop;
    Status s = XGetWMName(d, w, &prop);
    if (s) {
        int count = 0;
        char** list = NULL;
        int result = XmbTextPropertyToTextList(d, &prop, &list, &count);
        if (count && result == Success) {
            return list[0];
        } else {
            return "";
        }
    } else {
        return "";
    }
}

void x11_get_valid_windows(LinkedList* list) {
    static Display* display = NULL;
    static Window root = 0;
    static Window curr;
    if (!display) {
        display = XOpenDisplay(NULL);
    }
    if (!root) {
        root = DefaultRootWindow(display);
        curr = root;
    }
    Window parent;
    Window* children;
    unsigned int nchildren;
    XQueryTree(display, curr, &root, &parent, &children, &nchildren);
    // check the dimensions of each window
    XWindowAttributes attr;
    XGetWindowAttributes(display, curr, &attr);
    if (attr.x >= 0 && attr.y >= 0 && attr.width >= MIN_SCREEN_WIDTH && attr.height >= MIN_SCREEN_HEIGHT && *get_window_name(display, curr) != '\0') {
        LOG_INFO("Valid window %s has %d children, position %d, %d, dimensions %d x %d", get_window_name(display, curr),
                 nchildren, attr.x, attr.y, attr.width, attr.height);
        WhistWindow* valid_window = safe_malloc(sizeof(WhistWindow));
        valid_window->window = curr;
        linked_list_add_tail(list, valid_window);
    }
        
    if (nchildren != 0) {
        for (unsigned int i = 0; i < nchildren; i++) {
            curr = children[i];
            x11_get_valid_windows(list);
        }
    }
}

void x11_get_window_attributes(Window w, WhistWindowData* d) {
    static Display* display = NULL;
    static Window root = 0;
    if (!display) {
        display = XOpenDisplay(NULL);
    }
    if (!root) {
        root = DefaultRootWindow(display);
    }
    XWindowAttributes attr;
    XGetWindowAttributes(display, w, &attr);
    d->width = attr.width;
    d->height = attr.height;
    d->x = attr.x;
    d->y = attr.y;
}

void log_tree(X11CaptureDevice* device, Window w) {
    Window curr = w;
    Window root;
    Window parent;
    Window* children;
    unsigned int nchildren;

    XQueryTree(device->display, curr, &root, &parent, &children, &nchildren);
    XWindowAttributes attr;
    XGetWindowAttributes(device->display, curr, &attr);
    LOG_INFO("Current window %s has %d children, position %d, %d, dimensions %d x %d", get_window_name(device->display, curr),
             nchildren, attr.x, attr.y, attr.width, attr.height);
    if (nchildren != 0) {
        for (unsigned int i = 0; i < nchildren; i++) {
            LOG_INFO("Child %d name is %s", i, get_window_name(device->display, children[i]));
            log_tree(device, children[i]);
        }
    }
}

Window x11_get_active_window() {
    static Display* display = NULL;
    static Window root = 0;
    if (!display) {
        display = XOpenDisplay(NULL);
    }
    if (!root) {
        root = DefaultRootWindow(display);
    }
    static bool atom_set = false;
    static Atom net_active_window;
    if (!atom_set) {
        net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
        atom_set = true;
    }
    static Atom actual_type;
    static int actual_format;
    static unsigned long nitems, bytes_after;
    static char* result;
    if (XGetWindowProperty(display, root, net_active_window, 0, LONG_MAX / 4, False,
                           AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after,
                           (unsigned char**)&result) == Success &&
        *(unsigned long*)result != 0) {
        Window active_window = (Window) * (unsigned long*)result;
        // XFree(result);
        return active_window;
    } else {
        // revert to XGetInputFocus
        Window focus;
        int revert;
        XGetInputFocus(display, &focus, &revert);
        if (focus != PointerRoot) {
            return focus;
        } else {
            LOG_INFO("No active window found, setting root as active");
            return root;
        }
    }
}

/*
============================
Public Function Implementations
============================
*/
X11CaptureDevice* create_x11_capture_device(uint32_t width, uint32_t height, uint32_t dpi) {
    /*
        Create an X11 device that will capture a screen of the specified width, height, and DPI
       using the X11 API.

        Arguments:
            width (uint32_t): desired window width
            height (uint32_t): desired window height
            dpi (uint32_t): desired window DPI

        Returns:
            (X11CaptureDevice*): pointer to the created device
    */
    UNUSED(dpi);
    // malloc and 0-init the device
    X11CaptureDevice* device = (X11CaptureDevice*)safe_malloc(sizeof(X11CaptureDevice));
    memset(device, 0, sizeof(X11CaptureDevice));

    device->display = XOpenDisplay(NULL);
    if (!device->display) {
        LOG_ERROR("ERROR: create_x11_capture_device display did not open");
        return NULL;
    }
    device->root = DefaultRootWindow(device->display);
    device->width = width;
    device->height = height;
    int damage_event, damage_error;
    XDamageQueryExtension(device->display, &damage_event, &damage_error);
    device->damage = XDamageCreate(device->display, device->root, XDamageReportRawRectangles);
    device->event = damage_event;

    if (!reconfigure_x11_capture_device(device, width, height, dpi)) {
        return NULL;
    }

    return device;
}

bool reconfigure_x11_capture_device(X11CaptureDevice* device, uint32_t width, uint32_t height,
                                    uint32_t dpi) {
    if (device->image) {
        XFree(device->image);
        device->image = NULL;
    }
    device->width = width;
    device->height = height;
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
        LOG_ERROR("Error while getting window attributes");
        return false;
    }
    Screen* screen = window_attributes.screen;

    device->image =
        XShmCreateImage(device->display,
                        DefaultVisualOfScreen(screen),  // DefaultVisual(device->display, 0), // Use
                                                        // a correct visual. Omitted for brevity
                        DefaultDepthOfScreen(screen),   // 24,   // Determine correct depth from
                                                        // the visual. Omitted for brevity
                        ZPixmap, NULL, &device->segment, device->width, device->height);

    if (device->image == NULL) {
        LOG_ERROR("Could not XShmCreateImage!");
        return false;
    }

    device->segment.shmid = shmget(
        IPC_PRIVATE, device->image->bytes_per_line * device->image->height, IPC_CREAT | 0777);

    device->segment.shmaddr = device->image->data = shmat(device->segment.shmid, 0, 0);
    device->segment.readOnly = False;

    if (!XShmAttach(device->display, &device->segment)) {
        LOG_ERROR("Error while attaching display");
        destroy_x11_capture_device(device);
        return false;
    }
    device->frame_data = device->image->data;
    device->pitch = device->image->bytes_per_line;
    return true;
}

int x11_capture_screen(X11CaptureDevice* device) {
    /*
        Capture the screen using our X11 device. TODO: needs more documentation, I (Serina) am not
       really sure what's happening here.

        Arguments:
            device (X11CaptureDevice*): device used to capture the screen

        Returns:
            (int): 0 on success, -1 on failure
    */
    if (!device) {
        LOG_ERROR(
            "Tried to call x11_capture_screen with a NULL X11CaptureDevice! We shouldn't do this!");
        return -1;
    }

    int accumulated_frames = 0;
    while (XPending(device->display)) {
        // XDamageNotifyEvent* dev; unused, remove or is this needed and should
        // be used?
        XEvent ev;
        XNextEvent(device->display, &ev);
        if (ev.type == device->event + XDamageNotify) {
            // accumulated_frames will eventually be the number of damage events (accumulated
            // frames)
            accumulated_frames++;
        }
    }
    // Don't Lock and UnLock Display unneccesarily, if there are no frames to capture
    if (accumulated_frames == 0) return 0;

    device->first = true;
    XLockDisplay(device->display);
    if (accumulated_frames || device->first) {
        device->first = false;

        XDamageSubtract(device->display, device->damage, None, None);

        XWindowAttributes window_attributes;
        if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
            LOG_ERROR("Couldn't get window width and height!");
            accumulated_frames = -1;
        } else if (device->width != window_attributes.width ||
                   device->height != window_attributes.height) {
            LOG_ERROR("Wrong width/height!");
            accumulated_frames = -1;
        } else {
            XErrorHandler prev_handler = XSetErrorHandler(handler);
            if (!XShmGetImage(device->display, device->root, device->image, 0, 0, AllPlanes)) {
                LOG_ERROR("Error while capturing the screen");
                accumulated_frames = -1;
            } else {
                device->pitch = device->image->bytes_per_line;
            }
            if (accumulated_frames != -1) {
                // get the color
                XColor c;
                c.pixel = XGetPixel(device->image, 0, 0);
                XQueryColor(device->display,
                            DefaultColormap(device->display, XDefaultScreen(device->display)), &c);
                // Color format is r/g/b 0x0000-0xffff
                // We just need the leading byte
                device->corner_color.red = c.red >> 8;
                device->corner_color.green = c.green >> 8;
                device->corner_color.blue = c.blue >> 8;
            }
            XSetErrorHandler(prev_handler);
        }
    }
    XUnlockDisplay(device->display);
    return accumulated_frames;
}

void destroy_x11_capture_device(X11CaptureDevice* device) {
    /*
        Destroy the X11 device and free it.

        Arguments:
            device (X11CaptureDevice*): device to destroy
    */
    if (!device) {
        LOG_ERROR("Passed NULL into destroy_x11_capture_device!");
        return;
    }
    if (device->image) {
        XFree(device->image);
        device->image = NULL;
    }
    XCloseDisplay(device->display);
    free(device);
}
