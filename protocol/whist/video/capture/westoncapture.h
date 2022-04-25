#ifndef CAPTURE_WESTONCAPTURE_H
#define CAPTURE_WESTONCAPTURE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file westoncapture.h
 * @brief This file contains the code to do screen capture via the weston
============================
Usage
============================

*/

/*
============================
Includes
============================
*/

#include <stdint.h>
#include <stdbool.h>

#include <whist/core/whist.h>
#include <whist/utils/color.h>


#include "capture.h"

/*
============================
Custom Types
============================
*/


/*
============================
Public Functions
============================
*/
/**
 * @brief           Create a device for capturing via weston with the specified width, height, and DPI.
 *
 * @param infos     The desired width of the device
 * @returns         A pointer to the newly created WestonCaptureDevice
 */

CaptureDeviceImpl* create_weston_capture_device(CaptureDeviceInfos* infos, const char *display);

#if 0
bool reconfigure_weston_capture_device(WestonCaptureDevice* device, uint32_t width, uint32_t height,
                                    uint32_t dpi);



/**
 * @brief           Capture the screen with given device. Afterwards, the frame capture is stored in
 * frame_data.
 *
 * @param device    Device to use for screen captures
 *
 * @returns         0 on success, -1 on failure
 */
int weston_capture_screen(WestonCaptureDevice* device);

/**
 * @brief           Destroy the given WestonCaptureDevice and free it.
 *
 * @param device    Device to destroy
 */
void weston_destroy_capture_device(WestonCaptureDevice* device);
#endif


void whiston_inject_mouse_position(void *opaque, int x, int y);
void whiston_inject_mouse_buttons(void *opaque, int button, bool pressed);

#endif  // CAPTURE_WESTONCAPTURE_H
