/*
 * Audio encoding via FFmpeg library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H

#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>

#include "../core/fractal.h"

typedef struct {
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVFrame *pFrame;
    AVAudioFifo *pFifo;
    AVPacket packet;
    SwrContext *pSwrContext;
    int frame_count;

    int encoded_frame_size;
    void *encoded_frame_data;
} audio_encoder_t;

typedef struct {
    int pts;
    int size;
    uint8_t data[10];
} encoded_audio_t;

audio_encoder_t *create_audio_encoder(int bit_rate, int sample_rate);

void audio_encoder_fifo_intake(audio_encoder_t *encoder, uint8_t *data,
                               int len);

int audio_encoder_encode_frame(audio_encoder_t *encoder);

void destroy_audio_encoder(audio_encoder_t *encoder);

#endif  // ENCODE_H
