#ifndef DXGI_CUDA_CAPTURE_TRANSFER_H
#define DXGI_CUDA_CAPTURE_TRANSFER_H

// this is actually written in C++, so we have to include separately due to
// operator overloads within. we do this now so the rest of dxgicapture.h
// dependencies can be included together
#include <D3D11.h>

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

BEGIN_EXTERN_C
#include "dxgicapture.h"
#include "videoencode.h"

int dxgi_cuda_start_transfer_context(CaptureDevice* device);
void dxgi_cuda_close_transfer_context();
int dxgi_cuda_transfer_data(CaptureDevice* device, video_encoder_t* encoder);
END_EXTERN_C

#endif  // DXGI_CUDA_CAPTURE_TRANSFER_H