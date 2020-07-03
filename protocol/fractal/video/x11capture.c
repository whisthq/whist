/*
 * GPU screen capture on Linux Ubuntu.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "x11capture.h"

#include <X11/extensions/Xdamage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

#define USING_SHM true

int handler(Display* d, XErrorEvent* a) {
    mprintf("X11 Error: %d\n", a->error_code);
    return 0;
}

void get_wh(CaptureDevice* device, int* w, int* h) {
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root,
                              &window_attributes)) {
        mprintf("Error while getting window attributes\n");
        return;
    }
    *w = window_attributes.width;
    *h = window_attributes.height;
}

bool is_same_wh(CaptureDevice* device) {
    int w, h;
    get_wh(device, &w, &h);
    return device->width == w && device->height == h;
}

int CreateCaptureDevice(CaptureDevice* device, UINT width, UINT height) {
    device->display = XOpenDisplay(NULL);
    if (!device->display) {
        mprintf("ERROR: CreateCaptureDevice display did not open\n");
        return -1;
    }
    device->root = DefaultRootWindow(device->display);

    if (width <= 0 || height <= 0) {
        mprintf("Nonsensicle width/height of %d/%d\n", width, height);
        return -1;
    }
    device->width = width & ~0xF;
    device->height = height;

    if (!is_same_wh(device)) {
        system("xrandr --delmode default Fractal");
        system("xrandr --rmmode Fractal");

        char cmd[1000];
        sprintf(cmd,
                "xrandr --newmode Fractal $(cvt -r %d %d 60 | sed -n \"2p\" | "
                "cut -d' ' -f3-)",
                width, height);
        system(cmd);

        system("xrandr --addmode default Fractal");
        system("xrandr --output default --mode Fractal");

        // If it's still not the correct dimensions
        if (!is_same_wh(device)) {
            mprintf("Could not force monitor to a given width/height\n");
            get_wh(device, &device->width, &device->height);
        }
    }

    int damage_event, damage_error;
    XDamageQueryExtension(device->display, &damage_event, &damage_error);
    device->damage = XDamageCreate(device->display, device->root,
                                   XDamageReportRawRectangles);
    device->event = damage_event;

#if USING_SHM
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root,
                              &window_attributes)) {
        mprintf("Error while getting window attributes\n");
        return -1;
    }
    Screen* screen = window_attributes.screen;

    device->image = XShmCreateImage(
        device->display,
        DefaultVisualOfScreen(
            screen),  // DefaultVisual(device->display, 0), // Use a correct
                      // visual. Omitted for brevity
        DefaultDepthOfScreen(screen),  // 24,   // Determine correct depth from
                                       // the visual. Omitted for brevity
        ZPixmap, NULL, &device->segment, device->width, device->height);

    device->segment.shmid = shmget(
        IPC_PRIVATE, device->image->bytes_per_line * device->image->height,
        IPC_CREAT | 0777);

    device->segment.shmaddr = device->image->data =
        shmat(device->segment.shmid, 0, 0);
    device->segment.readOnly = False;

    if (!XShmAttach(device->display, &device->segment)) {
        mprintf("Error while attaching display\n");
        return -1;
    }
    device->frame_data = device->image->data;
    device->pitch = device->width * 4;
#else
    device->image = NULL;
    CaptureScreen(device);
#endif
    device->texture_on_gpu = false;

    return 0;
}

int CaptureScreen(CaptureDevice* device) {
    static bool first = true;

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

    if (update || first) {
        first = false;

        XDamageSubtract(device->display, device->damage, None, None);

        if (!is_same_wh(device)) {
            mprintf("Wrong width/height!\n");
            update = -1;
        } else {
            XErrorHandler prev_handler = XSetErrorHandler(handler);
#if USING_SHM
            if (!XShmGetImage(device->display, device->root, device->image, 0,
                              0, AllPlanes)) {
                mprintf("Error while capturing the screen");
                update = -1;
            }
#else
            if (device->image) {
                XFree(device->image);
            }
            device->image =
                XGetImage(device->display, device->root, 0, 0, device->width,
                          device->height, AllPlanes, ZPixmap);
            if (!device->image) {
                mprintf("Error while capturing the screen\n");
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

int TransferScreen(CaptureDevice* device) { return 0; }

void ReleaseScreen(CaptureDevice* device) {}

void DestroyCaptureDevice(CaptureDevice* device) {
    if (device->image) {
        XFree(device->image);
    }
    XCloseDisplay(device->display);
}
