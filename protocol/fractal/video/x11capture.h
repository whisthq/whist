#ifndef CAPTURE_X11CAPTURE_H
#define CAPTURE_X11CAPTURE_H

/**
 Copyright Fractal Computers, Inc. 2020
 @file x11capture.h
 @brief GPU screen capture on Linux Ubuntu.


============================
Usage
============================


*/




#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xdamage.h>
#include <stdbool.h>

#include "../core/fractal.h"

struct CaptureDevice {
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
  bool did_use_map_desktop_surface;
  bool released;
};

typedef unsigned int UINT;

int CreateCaptureDevice(struct CaptureDevice* device, UINT width, UINT height);

int CaptureScreen(struct CaptureDevice* device);

void ReleaseScreen(struct CaptureDevice* device);

void DestroyCaptureDevice(struct CaptureDevice* device);

#endif  // CAPTURE_X11CAPTURE_H
