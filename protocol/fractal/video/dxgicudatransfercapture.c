#include "../core/fractal.h"
#include "dxgicudatransfercapture.h"

// This file is used when CUDA cannot be found. See dxgicudatransfercapture.cu for the CUDA
// optimized transfer code

int dxgi_cuda_start_transfer_context(CaptureDevice* device) {
    device->dxgi_cuda_available = false;
    return 1;  // cuda unavailable
}

void dxgi_cuda_close_transfer_context(CaptureDevice* device) {
    device->dxgi_cuda_available = false;
}

int dxgi_cuda_transfer_capture(CaptureDevice* device, VideoEncoder* encoder) {
    return 1;  // cuda unavailable
}
