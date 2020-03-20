#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#include "fractal.h"  // contains all the headers

// define decoder struct to pass as a type
typedef struct {
    AVCodec *codec;
    AVCodecContext *context;
    AVFrame *sw_frame;
    AVFrame *hw_frame;
    AVPacket packet;
    DecodeType type;
    enum AVHWDeviceType device_type;
} video_decoder_t;

/// @brief creates encoder device
/// @details creates FFmpeg encoder
video_decoder_t *create_video_decoder(int in_width, int in_height,
                                      int out_width, int out_height,
                                      bool use_hardware);

/// @brief destroy decoder device
/// @details frees FFmpeg decoder memory
void destroy_video_decoder(video_decoder_t *decoder);

/// @brief decodes a frame using the decoder device
/// @details decode an encoded frame under YUV color format into RGB frame
bool video_decoder_decode(video_decoder_t *decoder, void *buffer,
                          int buffer_size);

#endif  // VIDEO_DECODE_H
