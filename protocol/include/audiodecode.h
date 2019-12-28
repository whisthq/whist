/*
 * This file contains the headers and definitions of the FFmpeg decoder functions
 * that decode the video frames.

 Protocol version: 1.0
 Last modification: 12/22/2019

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "fractal.h" // contains all the headers

// define decoder struct to pass as a type
typedef struct 
{
    AVCodec *codec;
    AVCodecContext *context;
    AVFrame *frame;
    AVPacket packet;
} audio_decoder_t;

/// @brief creates encoder device
/// @details creates FFmpeg encoder
audio_decoder_t *create_audio_decoder();

/// @brief destroy decoder device
/// @details frees FFmpeg decoder memory
void destroy_audio_decoder(audio_decoder_t *decoder);

/// @brief decodes a frame using the decoder device
/// @details decode an encoded frame under YUV color format into RGB frame
int audio_decoder_decode(audio_decoder_t *decoder, char *buffer, int buffer_size);

#endif // AUDIO_DECODE_H
