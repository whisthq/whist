/*
 * This file contains the implementation of the FFmpeg encoder functions
 * that encode the video frames.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdio.h>
#include <stdlib.h>

#include "videoencode.h" // header file for this file

/// @brief creates encoder encoder
/// @details creates FFmpeg encoder
encoder_t *create_video_encoder(int in_width, int in_height, int out_width, int out_height, int bitrate) {
  // set memory for the encoder
	encoder_t *encoder = (encoder_t *) malloc(sizeof(encoder_t));
	memset(encoder, 0, sizeof(encoder_t));

  // get frame dimensions for encoder
	encoder->in_width = in_width;
	encoder->in_height = in_height;
	encoder->out_width = out_width;
	encoder->out_height = out_height;

  // init ffmpeg encoder for H264 software encoding
	avcodec_register_all();
	encoder->codec = avcodec_find_encoder(AV_CODEC_ID_H264);

  // allocate and set ffmpeg context
	encoder->context = avcodec_alloc_context3(encoder->codec);
	encoder->context->dct_algo = FF_DCT_FASTINT;
	encoder->context->bit_rate = bitrate;
	encoder->context->width = out_width;
	encoder->context->height = out_height;
	encoder->context->time_base.num = 1;
	encoder->context->time_base.den = 30;
	encoder->context->gop_size = 1; // send SPS/PPS headers every packet
	encoder->context->max_b_frames = 1;
	encoder->context->pix_fmt = AV_PIX_FMT_YUV420P;

	// set encoder parameters to max performancen
	av_opt_set(encoder->context->priv_data, "preset", "ultrafast", 0);
    av_opt_set(encoder->context->priv_data, "tune", "zerolatency", 0);

  // open capture encoder
	avcodec_open2(encoder->context, encoder->codec, NULL);

  // alloc frame format
	encoder->frame = (AVFrame *) av_frame_alloc();
	encoder->frame->format = AV_PIX_FMT_YUV420P;
	encoder->frame->width  = out_width;
	encoder->frame->height = out_height;
	encoder->frame->pts = 0;

  // set frame size and allocate memory for it
	int frame_size = avpicture_get_size(AV_PIX_FMT_YUV420P, out_width, out_height);
	encoder->frame_buffer = malloc(frame_size);

  // fill picture with empty frame buffer
	avpicture_fill((AVPicture *) encoder->frame, (uint8_t *) encoder->frame_buffer, AV_PIX_FMT_YUV420P, out_width, out_height);

  // set sws context for color format conversion
	encoder->sws = sws_getContext(in_width, in_height, AV_PIX_FMT_RGB32, out_width, out_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, 0, 0, 0);

  // return created encoder
	return encoder;
}

/// @brief destroy encoder encoder
/// @details frees FFmpeg encoder memory
void destroy_video_encoder(encoder_t *encoder) {
  // check if encoder encoder exists
	if (encoder == NULL) {
    printf("Cannot destroy encoder encoder.\n");
    return;
  }

  // free the ffmpeg contextes
	sws_freeContext(encoder->sws);
	avcodec_close(encoder->context);

  // free the encoder context and frame
	av_free(encoder->context);
	av_free(encoder->frame);

  // free the buffer and encoder
	free(encoder->frame_buffer);
	free(encoder);
	return;
}

/// @brief encode a frame using the encoder encoder
/// @details encode a RGB frame into encoded format as YUV color
void *	(encoder_t *encoder, void *rgb_pixels) {
  // define input data to encoder
	uint8_t *in_data[1] = {(uint8_t *) rgb_pixels};
	int in_linesize[1] = {encoder->in_width * 4};

  // scale to the encoder format
	sws_scale(encoder->sws, in_data, in_linesize, 0, encoder->in_height, encoder->frame->data, encoder->frame->linesize);

	encoder->frame->pts++;

  // init packet to prepare encoding
	av_free_packet(&encoder->packet);
	av_init_packet(&encoder->packet);
	int success = 0; // boolean for success or failure of encoding

  // attempt to encode the frame
	avcodec_encode_video2(encoder->context, &encoder->packet, encoder->frame, &success);
  // if encoding succeeded
	if (success) {
      return;
	}
  return;
}
