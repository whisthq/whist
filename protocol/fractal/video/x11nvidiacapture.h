#ifndef CAPTURE_X11NVIDIACAPTURE_H
#define CAPTURE_X11NVIDIACAPTURE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>

typedef struct {
    NVFBC_SESSION_HANDLE fbc_handle;
    NVFBC_API_FUNCTION_LIST p_fbc_fn;
    int width;
    int height;
    CodecType codec_type;
} NvidiaCaptureDevice;

int create_nvidia_capture_device(NvidiaCaptureDevice* device, int bitrate, CodecType codec);
int nvidia_capture_screen(NvidiaCaptureDevice* device);
void destroy_nvidia_capture_device(NvidiaCaptureDevice* device);

#endif  // CAPTURE_X11NVIDIACAPTURE_H
