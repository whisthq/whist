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
#include "x11capture.h"
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                         Initialize or reinitialize the transfer context,
 *                                if it is needed for the given (device, encoder) pair.
 *                                This should be called when either the (device),
 *                                or the (encoder) changes.
 *
 * @param device                  The capture device being used
 *
 * @param encoder                 The encoder being used
 */
void reinitialize_transfer_context(CaptureDevice* device, VideoEncoder* encoder);

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
