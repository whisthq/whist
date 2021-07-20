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
#include "nvidiacapture.h"
#include "x11capture.h"

/*
============================
Custom Types
============================
*/

/**
 * @brief           Enum indicating whether we are using the Nvidia or X11 capture device. If we
 * discover a third option for capturing, update this enum and the CaptureDevice struct below.
 */
typedef enum CaptureDeviceType { NVIDIA_DEVICE, X11_DEVICE } CaptureDeviceType;

/**
 * @brief           Struct holding the global data needed for capturing: the type of capture device
 * we are using, information necessary for resizing, and pointers to the specific capture devices.
 * The implementations of the screencapture.h API internally decide what capture device to use.
 */
typedef struct CaptureDevice {
    CaptureDeviceType active_capture_device;
    // TODO: put the next four elements in some kind of resize context
    int width;
    int height;
    Display* display;
    Window root;
    NvidiaCaptureDevice* nvidia_capture_device;
    X11CaptureDevice* x11_capture_device;
} CaptureDevice;

#endif
