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

/**
 * @brief                          Will create a new nvidia encoder
 *
 * @param bitrate                  The bitrate to encode at (In bits per second)
 * @param codec                    Which codec type (h264 or h265) to use
 * @param out_width                Width of the output frame
 * @param out_height               Height of the output frame
 *
 * @returns                        The newly created nvidia encoder
 */
NvidiaEncoder* create_nvidia_encoder(int bitrate, CodecType codec, int out_width, int out_height);

/**
 * @brief                          Will reconfigure an nvidia encoder
 *
 * @param encoder                  The nvidia encoder to reconfigure
 * @param bitrate                  The new bitrate
 * @param codec                    The new codec
 * @param out_width                The new output width
 * @param out_height               The new output height
 *
 * @returns                        true on success, false on failure
 */
bool nvidia_reconfigure_encoder(NvidiaEncoder* encoder, int out_width, int out_height, int bitrate,
                                CodecType codec);

/**
 * @brief                          Put the input data into the nvidia encoder
 *
 * @param encoder                  The encoder to encode with
 * @param dw_texture               The GPU pointers needed to hold the captured frame
 * @param dw_tex_target            The GPU pointers needed to hold the captured frame
 * @param width                    The width of the inputted frame
 * @param height                   The height of the inputted frame
 *
 * @returns                        0 on success, else -1
 *                                 This function will return -1 if width/height do not match
 *                                 out_width/out_height, as the nvidia encoder does not support
 *                                 serverside scaling yet.
 */
int nvidia_encoder_frame_intake(NvidiaEncoder* encoder, uint32_t dw_texture, uint32_t dw_tex_target,
                                int width, int height);

/**
 * @brief                          Set the next frame to be an IDR-frame,
 *                                 with SPS/PPS headers included as well.
 *
 * @param encoder                  Encoder to be updated
 */
void nvidia_set_iframe(NvidiaEncoder* encoder);

/**
 * @brief                          Allow the next frame to be either an I-frame
 *                                 or not an i-frame
 *
 * @param encoder                  Encoder to be updated
 */
void nvidia_unset_iframe(NvidiaEncoder* encoder);

/**
 * @brief                          Encode the most recently intake'd frame
 *
 * @param encoder                  The encoder to encode with
 *
 * @returns                        0 on success, else -1
 */
int nvidia_encoder_encode(NvidiaEncoder* encoder);

/**
 * @brief                          Destroy the nvidia encoder
 *
 * @param encoder                  The encoder to destroy
 */
void destroy_nvidia_encoder(NvidiaEncoder* encoder);

#endif
