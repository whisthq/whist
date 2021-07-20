#ifndef CPU_CAPTURE_TRANSFER_H
#define CPU_CAPTURE_TRANSFER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file transfercapture.h
 * @brief This code handles the data transfer from a video capture device to a
 *        video encoder.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "videoencode.h"
#if defined(_WIN32)
#include "dxgicapture.h"
#else
#include "linuxcapture.h"
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                         Initialize the transfer context between (device) and (encoder).
 *                                Call this when we have changed resolution or recreated one of the
 *                                device or encoder.
 *
 * @param device                  The capture device being used
 *
 * @param encoder                 The encoder being used
 *
 * @returns                       0 on success, -1 on failure
 */
int start_transfer_context(CaptureDevice* device, VideoEncoder* encoder);

/**
 * @brief                         Close the transfer context between (device) and (encoder). Call
 *                                this when we have changed resolution or recreated one of the
 *                                device or encoder.
 *
 * @param device                  The capture device being used
 *
 * @param encoder                 The encoder being used
 *
 * @returns                       0 on success, -1 on failure
 */
int close_transfer_context(CaptureDevice* device, VideoEncoder* encoder);

/**
 * @brief                         Transfer the texture stored in the capture
 *                                device to the encoder, either via GPU or CPU
 *
 * @param device                  The capture device from which to get the
 *                                texture
 * @param encoder                 The encoder into which to load the frame data
 *
 * @returns                       0 on success, else -1
 */
int transfer_capture(CaptureDevice* device, VideoEncoder* encoder);

#endif  // CPU_CAPTURE_TRANSFER_H
