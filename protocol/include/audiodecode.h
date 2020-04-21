/*
 * Audio decoding via FFmpeg C library.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#define MAX_AUDIO_FRAME_SIZE 192000

#include "../include/fractal.h"
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

// define decoder struct to pass as a type
typedef struct {
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVFrame *pFrame;
    SwrContext *pSwrContext;
    uint8_t *out_buffer;
} audio_decoder_t;

audio_decoder_t *create_audio_decoder(int sample_rate);

int init_av_frame(audio_decoder_t *decoder);

int audio_decoder_get_frame_data_size(audio_decoder_t *decoder);

void audio_decoder_packet_readout(audio_decoder_t *decoder, uint8_t *data);

int audio_decoder_decode_packet(audio_decoder_t *decoder,
                                AVPacket *encoded_packet);

void destroy_audio_decoder(audio_decoder_t *decoder);

#endif  // DECODE_H
