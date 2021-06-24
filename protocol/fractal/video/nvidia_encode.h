#ifndef NVIDIA_ENCODE_H
#define NVIDIA_ENCODE_H

#include <fractal/core/fractal.h>

typedef struct {
    NV_ENC_REGISTERED_PTR registered_resources[NVFBC_TOGL_TEXTURES_MAX];
    NV_ENCODE_API_FUNCTION_LIST p_enc_fn;
    void* internal_nvidia_encoder;
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

int create_nvidia_encoder(NvidiaCaptureDevice* device, int bitrate, CodecType requested_codec,
                          NVFBC_TOGL_SETUP_PARAMS* p_setup_params, int out_width, int out_height);
int nvidia_encoder_frame_intake(VideoEncoder* encoder, void* input_buffer, int width, int height);
void nvidia_encoder_encode(NvidiaEncode* encoder);
void destroy_nvidia_encoder(NvidiaEncode* encoder);

#endif