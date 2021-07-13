#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file screencapture.h
 * @brief This file defines the proper header for capturing the screen depending
 *        on the local OS.
============================
Usage
============================

Toggles automatically between the screen capture files based on OS.
*/

/*
============================
Includes
============================
*/

// Defines the CaptureDevice type
#if defined(_WIN32)
#include "dxgicapture.h"
#else
#include "x11capture.h"
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Creates a screen capture device
 *
 * @param device                   Capture device struct to hold the capture
 *                                 device
 * @param width                    Width of the screen to capture, in pixels
 * @param height                   Height of the screen to capture, in pixels
 * @param dpi                      Dots per sq inch of the screen, (Where 96 is neutral)
 * @param p_cuda_context           Pointer to the CUDA context
 *
 * @returns                        0 if succeeded, else -1
 */
int create_capture_device(CaptureDevice* device, UINT width, UINT height, UINT dpi,
                          void** p_cuda_context);

/**
 * @brief                          Tries to reconfigure the capture device
 *                                 using the newly provided parameters
 *
 * @param device                   The Capture Device to reconfigure
 * @param width                    New width of the screen to capture, in pixels
 * @param height                   New height of the screen to capture, in pixels
 * @param dpi                      New DPI
 *
 * @returns                        0 if succeeded, else -1
 */
bool reconfigure_capture_device(CaptureDevice* device, UINT width, UINT height, UINT dpi);

/**
 * @brief                          Capture a bitmap snapshot of the screen
 *                                 The width/height of the image is guaranteed to be
 *                                 the width/height passed into create_/reconfigure_ capture device,
 *                                 If the screen dimensions changed, then -1 will be returned
 *
 * @param device                   The device used to capture the screen
 *
 * @returns                        Number of frames since last capture if succeeded, else -1
 */
int capture_screen(CaptureDevice* device);

/**
 * @brief                          Transfers screen capture to CPU buffer
 *                                 This is used if all attempts of a
 *                                 hardware screen transfer have failed
 *
 * @param device                   The capture device used to capture the screen
 *
 * @returns                        0 always
 */
int transfer_screen(CaptureDevice* device);

/**
 * @brief                          Release a captured screen bitmap snapshot
 *
 * @param device                   The capture device holding the
 *                                 screen object captured
 */
void release_screen(CaptureDevice* device);

/**
 * @brief                          Destroys and frees the memory of a capture device
 *
 * @param device                   The capture device to free
 */
void destroy_capture_device(CaptureDevice* device);

#endif  // SCREENCAPTURE_H
