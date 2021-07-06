#ifndef CAPTURE_X11CAPTURE_H
#define CAPTURE_X11CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file x11capture.h
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

/*
============================
Includes
============================
*/

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>
#include <stdbool.h>

#include <fractal/core/fractal.h>
#include "x11nvidiacapture.h"

/*
============================
Custom Types
============================
*/

typedef struct CaptureDevice {
    Display* display;
    XImage* image;
    XShmSegmentInfo segment;
    Window root;
    int counter;
    int width;
    int height;
    int pitch;
    char* frame_data;
    Damage damage;
    int event;
    bool texture_on_gpu;
    bool released;
    bool using_nvidia;
    NvidiaCaptureDevice nvidia_capture_device;
    bool capture_is_on_nvidia;
    // True if the first frame is the next to be captured
    bool first;
} CaptureDevice;

typedef unsigned int UINT;

#endif  // CAPTURE_X11CAPTURE_H
