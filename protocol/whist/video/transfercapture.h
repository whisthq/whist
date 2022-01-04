#ifndef TRANSFER_CAPTURE_H
#define TRANSFER_CAPTURE_H
/**
 * Copyright 2022 Whist Technologies, Inc.
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

#include "codec/encode.h"
#include "capture/capture.h"
#include <whist/utils/color.h>

/*
============================
Public Functions
============================
*/

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

#endif  // TRANSFER_CAPTURE_H
