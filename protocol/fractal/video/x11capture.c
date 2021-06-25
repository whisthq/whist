/**
 * Copyright Fractal Computers, Inc. 2021
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

#include "screencapture.h"

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
    if (!device) return;

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

int create_capture_device(CaptureDevice* device, UINT width, UINT height, UINT dpi, int bitrate,
                          CodecType codec) {
    if (!device) return -1;

    device->display = XOpenDisplay(NULL);
    if (!device->display) {
        LOG_ERROR("ERROR: CreateCaptureDevice display did not open");
        return -1;
    }
    device->root = DefaultRootWindow(device->display);

    if (width <= 0 || height <= 0) {
        LOG_ERROR("Invalid width/height of %d/%d", width, height);
        return -1;
    }
    device->width = width;
    device->height = height;

    if (!is_same_wh(device)) {
        char modename[128];
        char cmd[1024];

        snprintf(modename, sizeof(modename), "Fractal-%dx%d", width, height);

        char* display_name;
        runcmd("xrandr --current | grep \" connected\"", &display_name);
        *strchr(display_name, ' ') = '\0';

        snprintf(cmd, sizeof(cmd), "xrandr --delmode %s %s", display_name, modename);
        runcmd(cmd, NULL);
        snprintf(cmd, sizeof(cmd), "xrandr --rmmode %s", modename);
        runcmd(cmd, NULL);
        snprintf(cmd, sizeof(cmd),
                 "xrandr --newmode %s $(cvt -r %d %d 60 | sed -n \"2p\" | "
                 "cut -d' ' -f3-)",
                 modename, width, height);
        runcmd(cmd, NULL);
        snprintf(cmd, sizeof(cmd), "xrandr --addmode %s %s", display_name, modename);
        runcmd(cmd, NULL);
        snprintf(cmd, sizeof(cmd), "xrandr --output %s --mode %s", display_name, modename);
        runcmd(cmd, NULL);

        // For the single-monitor case, we no longer need to do the below; we instead
        // assume the display server has been created with the correct DPI. The below
        // approach does not handle this case well because Linux does not yet support
        // changing DPI on the fly. For getting seamless performance on multi-monitor
        // setups, we may eventually need to instead get already-running X11
        // applications to respect DPI changes to the X server.

        // I believe this command sets the DPI, as checked by `xdpyinfo | grep resolution`
        // snprintf(cmd, sizeof(cmd), "xrandr --dpi %d", dpi);
        // runcmd(cmd, NULL);
        // while this command sets the font DPI setting
        // snprintf(cmd, sizeof(cmd), "echo Xft.dpi: %d | xrdb -merge", dpi);
        // runcmd(cmd, NULL);

        free(display_name);

        // If it's still not the correct dimensions
        if (!is_same_wh(device)) {
            LOG_ERROR("Could not force monitor to a given width/height");
            get_wh(device, &device->width, &device->height);
        }
    }

#if USING_NVIDIA_CAPTURE_AND_ENCODE
    if (create_nvidia_capture_device(&device->nvidia_capture_device) == 0) {
        device->using_nvidia = true;
        device->image = NULL;
        LOG_INFO("Using Nvidia Capture SDK!");
        return 0;
    } else {
        device->using_nvidia = false;
        LOG_ERROR("USING_NVIDIA_CAPTURE_AND_ENCODE defined but unable to use Nvidia Capture SDK!");
        // Cascade to X11 capture below
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
    capture_screen(device);
#endif
    device->capture_is_on_nvidia = false;
    device->texture_on_gpu = false;

    return 0;
}

int capture_screen(CaptureDevice* device) {
    if (!device) return -1;

    if (device->using_nvidia) {
        int ret = nvidia_capture_screen(&device->nvidia_capture_device);
        if (ret < 0) {
            return ret;
        } else {
            device->frame_data = device->nvidia_capture_device.frame;
            device->width = device->nvidia_capture_device.width;
            device->height = device->nvidia_capture_device.height;
            device->pitch = device->width * 4;
            device->capture_is_on_nvidia = true;
            return ret;
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

int transfer_screen(CaptureDevice* device) { return 0; }

void release_screen(CaptureDevice* device) {}

void destroy_capture_device(CaptureDevice* device) {
    if (!device) return;

    if (device->using_nvidia) {
        destroy_nvidia_capture_device(&device->nvidia_capture_device);
    } else {
        if (device->image) {
            XFree(device->image);
        }
    }
    XCloseDisplay(device->display);
}

void update_capture_encoder(CaptureDevice* device, int bitrate, CodecType codec) {
    if (!device) return;

    if (device->using_nvidia) {
        destroy_nvidia_capture_device(&device->nvidia_capture_device);
        create_nvidia_capture_device(&device->nvidia_capture_device, bitrate, codec);
    }
}
