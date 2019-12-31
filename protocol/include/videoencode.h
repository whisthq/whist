/*
 * This file contains the headers and definitions of the FFmpeg encoder functions
 * that encode the video frames.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H

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
encoder_t *create_video_encoder(int in_width, int in_height, int out_width, int out_height, int bitrate);

/// @brief destroy encoder device
/// @details frees FFmpeg encoder memory
void destroy_video_encoder(encoder_t *encoder);

/// @brief encodes a frame using the encoder device
/// @details encodes a RGB frame into encoded format as YUV color
void *video_encoder_encode(encoder_t *encoder, void *rgb_pixels);

#endif // ENCODE_H
