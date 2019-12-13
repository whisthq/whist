/*
 * This file contains the headers and definitions of the FFmpeg encoder functions
 * that encode the video frames.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef ENCODE_H
#define ENCODE_H

#include "fractal.h" // contains all the headers

// define encoder struct to pass as a type
typedef struct {
	AVCodec *codec;
	AVCodecContext *context;
	AVFrame *frame;
	void *frame_buffer;
	int in_width, in_height;
	int out_width, out_height;
	AVPacket packet;
	struct SwsContext *sws;
} encoder_t;

/// @brief creates encoder device
/// @details creates FFmpeg encoder
encoder_t *create_encoder(int in_width, int in_height, int out_width, int out_height, int bitrate);

/// @brief destroy encoder device
/// @details frees FFmpeg encoder memory
void destroy_encoder(encoder_t *encoder);

/// @brief encode a frame using the encoder device
/// @details encode a RGB frame into encoded format as YUV color
void *encoder_encode(encoder_t *encoder, void *rgb_pixels, void *encoded_data, size_t *encoded_size);

#endif // ENCODE_H
