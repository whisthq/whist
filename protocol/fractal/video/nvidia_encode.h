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

    uint32_t dw_texture;
    uint32_t dw_tex_target;

    NV_ENC_INPUT_PTR input_buffer;
    NV_ENC_OUTPUT_PTR output_buffer;
    NV_ENC_BUFFER_FORMAT buffer_fmt;
    CodecType codec_type;
    uint32_t frame_idx;
    int width;
    int height;
    // Output
    void* frame;
    unsigned int frame_size;
    bool is_iframe;
} NvidiaEncoder;

NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType requested_codec,
                                     int out_width,
                                     int out_height);
// To be called by transfer_capture!
/// This should come from device->registered_resources[grab_params.dwTextureIndex];
int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, uint32_t dw_texture, uint32_t dw_tex_target);
// Encode the most recently provided frame from frame_intake
int nvidia_encoder_encode(NvidiaEncoder* encoder);
void destroy_nvidia_encoder(NvidiaEncoder* encoder);
int reconfigure_nvidia_encoder(NvidiaEncoder* encoder, int bitrate, CodecType codec);

#endif
