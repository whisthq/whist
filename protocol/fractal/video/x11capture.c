/**
 * Copyright Fractal Computers, Inc. 2021
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
#include "screencapture.h"

#include <X11/extensions/Xdamage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

/*
============================
Private Functions
============================
*/
int handler(Display* d, XErrorEvent* a);

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

#if USING_SHM
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
        LOG_ERROR("Error while getting window attributes");
        return NULL;
    }
    Screen* screen = window_attributes.screen;

    device->image =
        XShmCreateImage(device->display,
                        DefaultVisualOfScreen(screen),  // DefaultVisual(device->display, 0), // Use
                                                        // a correct visual. Omitted for brevity
                        DefaultDepthOfScreen(screen),   // 24,   // Determine correct depth from
                                                        // the visual. Omitted for brevity
                        ZPixmap, NULL, &device->segment, device->width, device->height);

    device->segment.shmid = shmget(
        IPC_PRIVATE, device->image->bytes_per_line * device->image->height, IPC_CREAT | 0777);

    device->segment.shmaddr = device->image->data = shmat(device->segment.shmid, 0, 0);
    device->segment.readOnly = False;

    if (!XShmAttach(device->display, &device->segment)) {
        LOG_ERROR("Error while attaching display");
        destroy_x11_capture_device(device);
        return NULL;
    }
    device->frame_data = device->image->data;
    device->pitch = device->width * 4;
#else
    device->image = NULL;
#endif
    return device;
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

    device->first = true;
    XLockDisplay(device->display);

    int update = 0;
    while (XPending(device->display)) {
        // XDamageNotifyEvent* dev; unused, remove or is this needed and should
        // be used?
        XEvent ev;
        XNextEvent(device->display, &ev);
        if (ev.type == device->event + XDamageNotify) {
            update = 1;
        }
    }

    if (update || device->first) {
        device->first = false;

        XDamageSubtract(device->display, device->damage, None, None);

        XWindowAttributes window_attributes;
        if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
            LOG_ERROR("Couldn't get window width and height!");
            update = -1;
        } else if (device->width != window_attributes.width ||
                   device->height != window_attributes.height) {
            LOG_ERROR("Wrong width/height!");
            update = -1;
        } else {
            XErrorHandler prev_handler = XSetErrorHandler(handler);
#if USING_SHM
            if (!XShmGetImage(device->display, device->root, device->image, 0, 0, AllPlanes)) {
                LOG_ERROR("Error while capturing the screen");
                update = -1;
            }
#else
            if (device->image) {
                XFree(device->image);
            }
            device->image = XGetImage(device->display, device->root, 0, 0, device->width,
                                      device->height, AllPlanes, ZPixmap);
            if (!device->image) {
                LOG_ERROR("Error while capturing the screen");
                update = -1;
            } else {
                device->frame_data = device->image->data;
                device->pitch = device->width * 4;
            }
#endif
            XSetErrorHandler(prev_handler);
        }
    }
    XUnlockDisplay(device->display);
    return update;
}

void destroy_x11_capture_device(X11CaptureDevice* device) {
    /*
        Destroy the X11 device and free it.

        Arguments:
            device (X11CaptureDevice*): device to destroy
    */
    if (!device) return;
    if (device->image) {
        XFree(device->image);
    }
    XCloseDisplay(device->display);
    free(device);
}
