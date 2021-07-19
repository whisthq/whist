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

/*
============================
Custom Types
============================
*/

typedef struct X11CaptureDevice {
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
    bool first;
} X11CaptureDevice;

X11CaptureDevice* create_x11_capture_device(uint32_t width, uint32_t height, uint32_t dpi);

int x11_capture_screen(X11CaptureDevice* device);

void destroy_x11_capture_device(X11CaptureDevice* device);

#endif  // CAPTURE_X11CAPTURE_H
