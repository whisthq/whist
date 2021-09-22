#ifndef DXGI_CUDA_TRANSFER_CAPTURE_H
#define DXGI_CUDA_TRANSFER_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file dxgicudatransfercapture.h
 * @brief This code handles the data transfer from a video capture device to a
 *        video encoder via DXGI CUDA.
============================
Usage
============================
*/

/*
============================
Defines
============================
*/

// magic -- when NVCC compiles this as C++, include things as C, else treat as C
#ifdef __cplusplus
#undef BEGIN_EXTERN_C
#undef END_EXTERN_C
#define BEGIN_EXTERN_C extern "C" {
#define END_EXTERN_C }
#else
#undef BEGIN_EXTERN_C
#undef END_EXTERN_C
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#endif

/*
============================
Includes
============================
*/

/*
 * this is actually written in C++, so we have to include separately due to
 * operator overloads within. we do this now so the rest of dxgicapture.h
 * dependencies can be included together
 */


BEGIN_EXTERN_C
#include <D3D11.h>
#include "dxgicapture.h"
#include "videoencode.h"
END_EXTERN_C

/*
============================
Public Functions
============================
*/

BEGIN_EXTERN_C

#ifdef FRACTAL_CUDA_ENABLED

/**
 * @brief                         Check CUDA capabilities and, if possible,
 *                                register the screen capture texture resource
 *                                for CUDA access buffer into the encoder's
 *                                input frame. Must be called after creating the
 *                                capture device
 *
 * @param device                  The capture device to register
 *
 * @returns                       0 on success, 1 if CUDA is unavailable, and -1
 *                                on error
 */
int dxgi_cuda_start_transfer_context(CaptureDevice* device);

/**
 * @brief                         Unregister the screen capture texture resource
 *                                for the capture device. Must be called before
 *                                destroying the capture device
 *
 * @param device                  The capture device to unregister
 */
void dxgi_cuda_close_transfer_context(CaptureDevice* device);

/**
 * @brief                         Transfer the texture stored in the capture
 *                                device to the hw_frame for an NVENC encoder.
 *                                The DXGI-CUDA transfer context must first be
 *                                started with
 *                                `dxgi_cuda_start_transfer_context`
 *
 * @param device                  The capture device that has a texture
 * @param encoder                 The NVENC encoder into which to load the frame
 * data
 *
 * @returns                       0 on success, 1 if CUDA is unavailable, and -1
 *                                on error
 */
int dxgi_cuda_transfer_capture(CaptureDevice* device, VideoEncoder* encoder);

#else

#define dxgi_cuda_start_transfer_context(...) (0)
#define dxgi_cuda_close_transfer_context(...) (0)
#define dxgi_cuda_transfer_capture(...) (0)

#endif

END_EXTERN_C

#endif  // DXGI_CUDA_TRANSFER_CAPTURE_H
