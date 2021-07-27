#ifndef CAPTURE_X11NVIDIACAPTURE_H
#define CAPTURE_X11NVIDIACAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file nvidiacapture.h
 * @brief This file contains the code to do screen capture via the Nvidia FBC SDK on Linux Ubuntu.
============================
Usage
============================

NvidiaCaptureDevice contains all the information used to interface with the Nvidia FBC SDK and the
data of a frame. Call create_nvidia_capture_device to initialize a device, nvidia_capture_screen to
capture the screen with said device, and destroy_nvidia_capture_device when done capturing frames.
*/

/*
============================
Includes
============================
*/
#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>
#include "cudacontext.h"

/*
============================
Custom Types
============================
*/

/**
 * @brief       Struct to handle using Nvidia for capturing the screen. p_fbc_fn contains pointers
 * to the API functions, and dw_texture_index keeps track of the GL texture that the capture is
 * stored on.
 */
typedef struct {
    // Width and height of the capture session
    int width;
    int height;

    // The gpu texture of the most recently captured frame
    void* p_gpu_texture;

    // Internal Nvidia API structs
    NVFBC_SESSION_HANDLE fbc_handle;
    NVFBC_API_FUNCTION_LIST p_fbc_fn;
} NvidiaCaptureDevice;

/*
============================
Public Functions
============================
*/
/**
 * @brief                          Creates an nvidia capture device
 *
 * @param device                   Capture device struct to hold the capture
 *                                 device
 *
 * @returns                        0 on success, -1 on failure
 */
NvidiaCaptureDevice* create_nvidia_capture_device();

/**
 * @brief                          Captures the screen into GPU pointers.
 *                                 This will also set width/height to the dimensions
 *                                 of the captured image.
 *
 * @param device                   Capture device to capture with
 *
 * @returns                        0 on success, -1 on failure
 */
int nvidia_capture_screen(NvidiaCaptureDevice* device);

/**
 * @brief                          Destroy the nvidia capture device
 *
 * @param device                   The Capture device to destroy
 */
void destroy_nvidia_capture_device(NvidiaCaptureDevice* device);

#endif  // CAPTURE_X11NVIDIACAPTURE_H
