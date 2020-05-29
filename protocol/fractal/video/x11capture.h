#ifndef CAPTURE_X11CAPTURE_H
#define CAPTURE_X11CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file x11capture.h
 * @brief This file contains the code to do GPU screen capture on Linux Ubuntu.
============================
Usage
============================
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

#include "../core/fractal.h"

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
  bool did_use_map_desktop_surface;
  bool released;
} CaptureDevice;

typedef unsigned int UINT;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Create a XSHM screen capture device
 *
 * @param device                   Capture device struct to hold the capture device
 * @param width                    Width of the screen to capture, in pixels
 * @param height                   Height of the screen to capture, in pixels
 *
 * @returns                        0 if succeeded, else -1     
 */
int CreateCaptureDevice(CaptureDevice* device, UINT width, UINT height);

/**
 * @brief                          Capture a bitmap snapshot of the screen, in the GPU, using XSHM
 *
 * @param device                   The device used to capture the screen
 *
 * @returns                        0 if succeeded, else -1     
 */
int CaptureScreen(CaptureDevice* device);

/**
 * @brief                          Release a captured screen bitmap snapshot
 *
 * @param device                   The Linux screencapture device holding the screen object captured
 */
void ReleaseScreen(CaptureDevice* device);

/**
 * @brief                          Destroys and frees the memory of a Linux Ubuntu screencapture device
 *
 * @param device                   The Linux Ubuntu screencapture device to destroy and free the memory of
 */
void DestroyCaptureDevice(CaptureDevice* device);

#endif  // CAPTURE_X11CAPTURE_H
