/**
 * Copyright Fractal Computers, Inc. 2020
 * @file x11capture.c
 * @brief This file contains the code to do screen capture in the GPU on Linux
 *        Ubuntu.
============================
Usage
============================

CaptureDevice contains all the information used to interface with the X11 screen
capture API and the data of a frame.

Call CreateCaptureDevice to initialize at the beginning of the program, and
DestroyCaptureDevice to clean up at the end of the program. Call CaptureScreen
to capture a frame.

You must release each frame you capture via ReleaseScreen before calling
CaptureScreen again.
*/

#include "x11capture.h"

#include <X11/extensions/Xdamage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

int handler(Display* d, XErrorEvent* a) {
    LOG_ERROR("X11 Error: %d", a->error_code);
    return 0;
}

void get_wh(CaptureDevice* device, int* w, int* h) {
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
	*w = 0;
	*h = 0;
        LOG_ERROR("Error while getting window attributes");
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
        LOG_ERROR("ERROR: CreateCaptureDevice display did not open");
        return -1;
    }
    device->root = DefaultRootWindow(device->display);

    if (width <= 0 || height <= 0) {
        LOG_ERROR("Nonsensicle width/height of %d/%d", width, height);
        return -1;
    }
    device->width = width & ~0xF;
    device->height = height;

    if (!is_same_wh(device)) {
	char modename[1024];
	char cmd[1024];

	snprintf(modename, sizeof(modename), "Fractal-%dx%d", width, height);

	snprintf(cmd, sizeof(cmd), "xrandr --delmode default %s", modename);
        system(cmd);
	snprintf(cmd, sizeof(cmd), "xrandr --delmode DVI-D-0 %s", modename);
        system(cmd);
        snprintf(cmd, sizeof(cmd), "xrandr --rmmode %s", modename);
        system(cmd);

        snprintf(cmd, sizeof(cmd),
                "xrandr --newmode %s $(cvt -r %d %d 60 | sed -n \"2p\" | "
                "cut -d' ' -f3-)",
                modename, width, height);
        system(cmd);

	snprintf(cmd, sizeof(cmd), "xrandr --addmode default %s", modename);
        system(cmd);
	snprintf(cmd, sizeof(cmd), "xrandr --output default --mode %s", modename);
        system(cmd);
	snprintf(cmd, sizeof(cmd), "xrandr --addmode DVI-D-0 %s", modename);
        system(cmd);
	snprintf(cmd, sizeof(cmd), "xrandr --output DVI-D-0 --mode %s", modename);
        system(cmd);

        // If it's still not the correct dimensions
        if (!is_same_wh(device)) {
            LOG_ERROR("Could not force monitor to a given width/height");
            get_wh(device, &device->width, &device->height);
        }
    }

#if USING_GPU_CAPTURE
    if (CreateNvidiaCaptureDevice(&device->nvidia_capture_device) < 0) {
	device->using_nvidia = false;
    } else {
	device->using_nvidia = true;
	return 0;
    }
#endif

    int damage_event, damage_error;
    XDamageQueryExtension(device->display, &damage_event, &damage_error);
    device->damage = XDamageCreate(device->display, device->root, XDamageReportRawRectangles);
    device->event = damage_event;

#if USING_SHM
    XWindowAttributes window_attributes;
    if (!XGetWindowAttributes(device->display, device->root, &window_attributes)) {
        LOG_ERROR("Error while getting window attributes");
        return -1;
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
    if (device->using_nvidia) {
        int ret = NvidiaCaptureScreen(&device->nvidia_capture_device);
	if (ret < 0) {
	    return ret;
	} else {
	    device->frame_data = device->nvidia_capture_device.frame;
	    device->pitch = device->width * 4;
	    return 1;
	}
    }

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

int TransferScreen(CaptureDevice* device) { return 0; }

void ReleaseScreen(CaptureDevice* device) {}

void DestroyCaptureDevice(CaptureDevice* device) {
    if (device->using_nvidia) {
	DestroyNvidiaCaptureDevice(&device->nvidia_capture_device);
    } else {
	if (device->image) {
            XFree(device->image);
        }
    }
    XCloseDisplay(device->display);
}

void UpdateHardwareEncoder(CaptureDevice* device) {
    if (device->using_nvidia) {
	DestroyNvidiaCaptureDevice(&device->nvidia_capture_device);
	CreateNvidiaCaptureDevice(&device->nvidia_capture_device);
    }
}

