#ifndef CAPTURE_X11NVIDIACAPTURE_H
#define CAPTURE_X11NVIDIACAPTURE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>
#include <fractal/utils/metrics.h>

typedef struct NvidiaCaptureDevice {
    NVFBC_SESSION_HANDLE fbc_handle;
    NVFBC_API_FUNCTION_LIST p_fbc_fn;
    NV_ENC_REGISTERED_PTR registered_resources[NVFBC_TOGL_TEXTURES_MAX];
    void* frame;
    unsigned int size;
    bool is_iframe;
    int width;
    int height;
    CodecType codec_type;
    // Encoder Stuff
    NV_ENCODE_API_FUNCTION_LIST p_enc_fn;
    void* encoder;
    NV_ENC_INPUT_PTR input_buffer;
    NV_ENC_OUTPUT_PTR output_buffer;
    int metrics_ids[MAX_NUM_TRACKED_PACKETS];
    uint32_t frame_idx;

} NvidiaCaptureDevice;

int create_nvidia_capture_device(NvidiaCaptureDevice* device, int bitrate, CodecType codec);
int nvidia_capture_screen(NvidiaCaptureDevice* device);
void destroy_nvidia_capture_device(NvidiaCaptureDevice* device);

#endif  // CAPTURE_X11NVIDIACAPTURE_H
