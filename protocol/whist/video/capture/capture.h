#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file capture.h
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

#include <whist/core/whist.h>
#include <whist/utils/color.h>
#include <whist/utils/linked_list.h>
#include <whist/video/nvidia-linux/cuda_drvapi_dynlink_cuda.h>
#if OS_IS(OS_LINUX)
#include <X11/Xlib.h>
#endif

/*
============================
Custom Types
============================
*/
typedef struct CaptureDevice CaptureDevice;
typedef struct CaptureDeviceImpl CaptureDeviceImpl;

typedef int (*CaptureDeviceInitFn)(CaptureDeviceImpl* device, uint32_t width, uint32_t height,
                                   uint32_t dpi);
typedef int (*CaptureDeviceReconfigureFn)(CaptureDeviceImpl* device, uint32_t width,
                                          uint32_t height, uint32_t dpi);
typedef int (*CaptureDeviceCaptureScreenFn)(CaptureDeviceImpl* device, uint32_t* width,
                                            uint32_t* height, uint32_t* pitch);
typedef int (*CaptureDeviceTransferScreenFn)(CaptureDeviceImpl* device);
typedef int (*CaptureDeviceGetDataFn)(CaptureDeviceImpl* device, void** buf, uint32_t* stride);
typedef int (*CaptureDeviceGetDimensionsFn)(CaptureDeviceImpl* device, uint32_t* width,
                                            uint32_t* height, uint32_t* dpi);
typedef int (*CaptureDeviceUpdateDimensionsFn)(CaptureDeviceImpl* device, uint32_t width,
                                               uint32_t height, uint32_t dpi);
typedef int (*CaptureDeviceCudaContext)(CaptureDeviceImpl* device, CUcontext* context);
typedef void (*CaptureDeviceDestroyFn)(CaptureDeviceImpl** device);

/** @brief main info about a capture device */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t dpi;
    uint32_t pitch;
    CaptureDeviceType last_capture_device;
} CaptureDeviceInfos;

/** @brief a capture implementation */
struct CaptureDeviceImpl {
    CaptureDeviceType device_type;
    CaptureDeviceInfos* infos;

    CaptureDeviceInitFn init;
    CaptureDeviceReconfigureFn reconfigure;
    CaptureDeviceCaptureScreenFn capture_screen;
    CaptureDeviceTransferScreenFn transfer_screen;
    CaptureDeviceGetDataFn capture_get_data;
    CaptureDeviceGetDimensionsFn get_dimensions;
    CaptureDeviceUpdateDimensionsFn update_dimensions;
    CaptureDeviceCudaContext cuda_context;
    CaptureDeviceDestroyFn destroy;
};

typedef struct CaptureDevice {
    CaptureDeviceInfos infos;
    void* frame_data;
    WhistWindow window_data[MAX_WINDOWS];
    WhistRGBColor corner_color;
    void* internal;
    CaptureDeviceImpl* impl;
} CaptureDevice;

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
 * @param capture_type             Kind of capture device
 * @param data                     An opaque data pointer given to the device
 * @param width                    Width of the screen to capture, in pixels
 * @param height                   Height of the screen to capture, in pixels
 * @param dpi                      Dots per sq inch of the screen, (Where 96 is neutral)
 *
 * @returns                        0 if succeeded, else -1
 */
int create_capture_device(CaptureDevice* device, CaptureDeviceType capture_type, void* data,
                          uint32_t width, uint32_t height, uint32_t dpi);

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
bool reconfigure_capture_device(CaptureDevice* device, uint32_t width, uint32_t height,
                                uint32_t dpi);

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
 * @brief                          Destroys and frees the memory of a capture device
 *
 * @param device                   The capture device to free
 */
void destroy_capture_device(CaptureDevice* device);

int capture_get_data(CaptureDevice* device, void** buf, uint32_t* stride);

#endif  // VIDEO_CAPTURE_H
