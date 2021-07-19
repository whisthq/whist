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


int create_capture_device(X11CaptureDevice* device, UINT width, UINT height, UINT dpi) {
    /*
        Create a device that will capture a screen of the specified width, height, and DPI.
        This function first attempts to use X11 to set
        the display's width, height, and DPI, then creates either an NVidia or X11 capture device,
        with NVidia given priority. Refer to x11nvidiacapture.c for the internal details of the
        NVidia capture device.

        Arguments:
            device (CaptureDevice*): the created capture device
            width (UINT): desired window width
            height (UNIT): desired window height
            dpi (UINT): desired window DPI

        Returns:
            (int): 0 on success, 1 on failure
    */

    // Create the NVidia capture device is possible; otherwise use X11 capture.

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
        destroy_capture_device(device);
        return -1;
    }
    device->frame_data = device->image->data;
    device->pitch = device->width * 4;
#else
    device->image = NULL;
    if (capture_screen(device) < 0) {
        LOG_ERROR("Failed to call capture_screen for the first frame!");
        destroy_capture_device(device);
        return -1;
    }
#endif
    device->capture_is_on_nvidia = false;
    device->texture_on_gpu = false;

    return 0;
}

bool reconfigure_capture_device(X11CaptureDevice* device, UINT width, UINT height, UINT dpi) {
    if (device == NULL) {
        LOG_ERROR("NULL device was passed into reconfigure_capture_device!");
        return false;
    }

    if (device->using_nvidia) {
        // Reconfigure nvidia device
        try_update_dimensions(device, width, height, dpi);
        return true;
    } else {
        // Can't reconfigure X11 device
        return false;
    }
}

int capture_screen(X11CaptureDevice* device) {
    /*
        Capture the screen using device. If using NVidia, we use the NVidia FBC API to capture the
        screen, as described in x11nvidiacapture.c. Otherwise, use X11 functions to capture the
        screen.

        Arguments:
            device (CaptureDevice*): device used to capture the screen

        Returns:
            (int): 0 on success, -1 on failure
    */
    if (!device) {
        LOG_ERROR("Tried to call capture_screen with a NULL CaptureDevice! We shouldn't do this!");
        return -1;
    }

    if (device->using_nvidia) {
        int ret = nvidia_capture_screen(&device->nvidia_capture_device);
        if (ret < 0) {
            LOG_ERROR("nvidia_capture_screen failed!");
            return ret;
        } else {
            // Ensure that we captured the width/height that we intended to
            if (device->width != device->nvidia_capture_device.width ||
                device->height != device->nvidia_capture_device.height) {
                LOG_ERROR(
                    "Capture Device dimensions %dx%d does not match nvidia dimensions of %dx%d!",
                    device->width, device->height, device->nvidia_capture_device.width,
                    device->nvidia_capture_device.height);
                return -1;
            }
            device->pitch = 0;
            device->frame_data = NULL;
            device->capture_is_on_nvidia = true;
            return ret;
        }
    }

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

void destroy_capture_device(X11CaptureDevice* device) {
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
