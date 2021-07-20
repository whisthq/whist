#ifndef NVIDIA_TRANSFER_CAPTURE_H
#define NVIDIA_TRANSFER_CAPTURE_H

/**
 * Copyright Fractal Computers, Inc. 2021
 * @file nvidiatransfercapture.h
 * @brief This code handles the data transfer, i.e. resource registration, from a video capture
 *        device to a video encoder via Nvidia's FBC and Video Codec SDKs.
============================
Usage
============================
When updating the device or encoder during a resolution change, or recreating the device or encoder,
call close_transfer_context to unregister any old resources, then start_transfer_context to register
new resources. Not unregistering resources can result in memory leaks and other strange behavior!
*/

/*
============================
Includes
============================
*/
#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include "nvidia_encode.h"
#include "nvidiacapture.h"

/*
============================
Public Functions
============================
*/
/**
 * @brief Register the device's GL capture resources with the encoder.
 *
 * @param device The Nvidia screen capture device
 *
 * @param encoder The Nvidia encoder for encoding screen captures
 *
 * @return 0 on success, -1 on failure
 */
int nvidia_start_transfer_context(NvidiaCaptureDevice* device, NvidiaEncoder* encoder);

/**
 * @brief Unregister the GL capture resources from the encoder.
 *
 * @param encoder The Nvidia encoder that had previously registered resources.
 *
 * @return 0 on success, -1 on failure
 */
int nvidia_close_transfer_context(NvidiaEncoder* encoder);

#endif
