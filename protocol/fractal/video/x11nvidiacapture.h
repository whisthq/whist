#ifndef CAPTURE_X11NVIDIACAPTURE_H
#define CAPTURE_X11NVIDIACAPTURE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>

typedef struct {
    NVFBC_SESSION_HANDLE fbc_handle;
    NVFBC_API_FUNCTION_LIST p_fbc_fn;
    // Contains pointers to the opengl textures that the Nvidia Capture Device will capture into
    NVFBC_TOGL_SETUP_PARAMS togl_setup_params;
    uint32_t dw_texture;
    uint32_t dw_tex_target;

    int width;
    int height;
} NvidiaCaptureDevice;

int create_nvidia_capture_device(NvidiaCaptureDevice* device);
int nvidia_capture_screen(NvidiaCaptureDevice* device);
void destroy_nvidia_capture_device(NvidiaCaptureDevice* device);

#endif  // CAPTURE_X11NVIDIACAPTURE_H
