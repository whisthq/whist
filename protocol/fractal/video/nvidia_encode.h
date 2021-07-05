#ifndef NVIDIA_ENCODE_H
#define NVIDIA_ENCODE_H

#include "nvidia-linux/NvFBCUtils.h"
#include "nvidia-linux/nvEncodeAPI.h"
#include <fractal/core/fractal.h>

typedef struct {
    // Width+Height of the GPU Texture
    int width;
    int height;
    // GPU pointers to the GPU texture
    uint32_t dw_texture;
    uint32_t dw_tex_target;
    // Encoder's pointer to the GPU texture,
    // so that the encoder can borrow that texture
    // This has to be generated from the above 4,
    // And has to be unregistered after use
    NV_ENC_REGISTERED_PTR registered_resource;
} InputBufferCacheEntry;

typedef struct {
    NV_ENCODE_API_FUNCTION_LIST p_enc_fn;
    void* internal_nvidia_encoder;
    NV_ENC_INITIALIZE_PARAMS encoder_params;

    NV_ENC_REGISTERED_PTR registered_resource;
    // We'll make the cache a bit bigger than it needs to be to ensure that it still works
    InputBufferCacheEntry registered_resources[NVFBC_TOGL_TEXTURES_MAX * 2];

    NV_ENC_INPUT_PTR input_buffer;
    NV_ENC_OUTPUT_PTR output_buffer;
    NV_ENC_BUFFER_FORMAT buffer_fmt;
    CodecType codec_type;
    uint32_t frame_idx;
    int width;
    int height;
    bool wants_iframe;
    // Output
    void* frame;
    unsigned int frame_size;
    bool is_iframe;
} NvidiaEncoder;

NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType requested_codec, int out_width,
                                     int out_height);
// Take in the GPU Texture pointer
int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, uint32_t dw_texture, uint32_t dw_tex_target,
                                int width, int height);
// Reconfigure the encoder
bool nvidia_reconfigure_encoder(NvidiaEncoder* encoder, int width, int height, int bitrate,
                                CodecType codec);
// Set next frame to be iframe
void nvidia_set_iframe(NvidiaEncoder* encoder);
// Unset next frame to be iframe
void nvidia_unset_iframe(NvidiaEncoder* encoder);
// Encode the most recently provided frame from frame_intake
int nvidia_encoder_encode(NvidiaEncoder* encoder);
// Destroy the nvidia encoder
void destroy_nvidia_encoder(NvidiaEncoder* encoder);

#endif
