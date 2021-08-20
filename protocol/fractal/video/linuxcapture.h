#ifndef LINUX_CAPTURE_H
#define LINUX_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file linuxcapture.h
 * @brief This file contains the code to create a capture device on a Linux machine using X11, and
use that device to capture frames. It implements screencapture.h.
============================
Usage
============================
We first try to create a capture device that uses Nvidia's FBC SDK for capturing frames. This
capture device must be paired with an Nvidia encoder. If those fail, we fall back to using X11's API
to create a capture device, which captures on the CPU, and encode using FFmpeg instead. See
videoencode.h for more details on encoding. The type of capture device currently in use is indicated
in active_capture_device.
*/

/*
============================
Includes
============================
*/
#include <X11/Xlib.h>
#include <fractal/core/fractal.h>
#include <fractal/utils/color.h>
#include "nvidiacapture.h"
#include "x11capture.h"

/*
============================
Custom Types
============================
*/

/**
 * @brief           Struct holding the global data needed for capturing: the type of capture device
 * we are using, information necessary for resizing, and pointers to the specific capture devices.
 * The implementations of the screencapture.h API internally decide what capture device to use.
 */
typedef struct CaptureDevice {
    CaptureDeviceType active_capture_device;  // the device currently used for capturing
    CaptureDeviceType last_capture_device;  // the device used for the last capture, so we can pick
                                            // the right encoder
    bool pending_destruction;
    FractalThread nvidia_manager;
    FractalSemaphore nvidia_device_semaphore;
    bool nvidia_context_is_stale;
    int width;
    int height;

    // To hold software captured frames
    void* frame_data;
    int pitch;
    FractalRGBColor corner_color;

    // Shared X11 state
    Display* display;
    Window root;

    // Underlying X11/Nvidia capture devices
    NvidiaCaptureDevice* nvidia_capture_device;
    X11CaptureDevice* x11_capture_device;
} CaptureDevice;

#endif
