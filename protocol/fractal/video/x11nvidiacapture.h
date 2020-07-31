#ifndef CAPTURE_X11NVIDIACAPTURE_H
#define CAPTURE_X11NVIDIACAPTURE_H

#include "nvidia-linux/NvFBCUtils.h"

typedef struct NvidiaCaptureDevice {
    NVFBC_SESSION_HANDLE fbcHandle;
    NVFBC_API_FUNCTION_LIST pFn;
    void *frame;
    unsigned int size;
} NvidiaCaptureDevice;

int CreateNvidiaCaptureDevice(NvidiaCaptureDevice* device);
int NvidiaCaptureScreen(NvidiaCaptureDevice* device);
void DestroyNvidiaCaptureDevice(NvidiaCaptureDevice* device);

#endif  // CAPTURE_X11NVIDIACAPTURE_H
