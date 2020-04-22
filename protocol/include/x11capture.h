/*
 * GPU screen capture on Linux Ubuntu.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef CAPTURE_X11CAPTURE_H
#define CAPTURE_X11CAPTURE_H

#include "fractal.h"
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>

struct CaptureDevice {
    Display* display;
    XImage* image;
    XShmSegmentInfo segment;
    Window root;
    int counter;
    int width;
    int height;
    char* frame_data;
    Damage damage;
    int event;
    bool did_use_map_desktop_surface;
    bool released;
};

typedef unsigned int UINT;

int CreateCaptureDevice( struct CaptureDevice* device, UINT width, UINT height );

int CaptureScreen( struct CaptureDevice* device );

void ReleaseScreen( struct CaptureDevice* device );

void DestroyCaptureDevice( struct CaptureDevice* device );

#endif // CAPTURE_X11CAPTURE_H
