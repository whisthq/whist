/*
 * Video decoding via FFmpeg library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#include "../core/fractal.h"

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

video_decoder_t *create_video_decoder(int width, int height, bool use_hardware);

void destroy_video_decoder(video_decoder_t *decoder);

bool video_decoder_decode(video_decoder_t *decoder, void *buffer,
                          int buffer_size);

#endif  // VIDEO_DECODE_H
