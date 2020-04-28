/*
 * Video encoding via FFmpeg C library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H

#include "../core/fractal.h"

// define encoder struct to pass as a type
typedef struct {
    AVCodec *codec;
    AVCodecContext *context;
    AVFrame *sw_frame;
    AVFrame *hw_frame;
    void *frame_buffer;
    int width, height;
    AVPacket packet;
    struct SwsContext *sws;
    EncodeType type;
} encoder_t;

encoder_t *create_video_encoder(int width, int height, int bitrate,
                                int gop_size);

void destroy_video_encoder(encoder_t *encoder);

void video_encoder_encode(encoder_t *encoder, void *rgb_pixels);

void video_encoder_set_iframe(encoder_t *encoder);

void video_encoder_unset_iframe(encoder_t *encoder);

#endif  // ENCODE_H
