#ifndef CPU_CAPTURE_TRANSFER_H
#define CPU_CAPTURE_TRANSFER_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file cpucapturetransfer.h
 * @brief This code handles the data transfer from a video capture device to a
 *        video encoder via the CPU.
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
 * @brief                         Transfer the texture stored in the capture
 *                                device to a CPU buffer, and load that CPU
 *                                buffer into the encoder's input frame
 *
 * @param device                  The capture device from which to get the
 *                                texture
 * @param encoder                 The encoder into which to load the frame data
 *
 * @returns                       0 on success, else -1
 */
int cpu_transfer_capture(CaptureDevice* device, video_encoder_t* encoder);

#endif  // CPU_CAPTURE_TRANSFER_H
