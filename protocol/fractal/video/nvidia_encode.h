#ifndef NVIDIA_ENCODE_H
#define NVIDIA_ENCODE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>

typedef struct {
    NV_ENC_REGISTERED_PTR registered_resources[NVFBC_TOGL_TEXTURES_MAX];
    NV_ENCODE_API_FUNCTION_LIST p_enc_fn;
    void* internal_nvidia_encoder;
    NV_ENC_INITIALIZE_PARAMS encoder_params;
    NV_ENC_INPUT_PTR input_buffer;
    NV_ENC_OUTPUT_PTR output_buffer;
    uint32_t frame_idx;
    int width;
    int height;
    // Output
    void* frame;
    unsigned int size;
    bool is_iframe;
} NvidiaEncoder;

NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType requested_codec,
                          NVFBC_TOGL_SETUP_PARAMS* p_setup_params, int out_width, int out_height);
int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, void* input_buffer, int width, int height);
void nvidia_encoder_encode(NvidiaEncoder* encoder);
void destroy_nvidia_encoder(NvidiaEncoder* encoder);

#endif
