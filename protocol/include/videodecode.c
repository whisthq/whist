/*
 * This file contains the implementation of the FFmpeg decoder functions
 * that decode the video frames.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdio.h>
#include <stdlib.h>

#include "videodecode.h" // header file for this file

/// @brief creates decoder decoder
/// @details creates FFmpeg decoder
decoder_t *create_video_decoder(int in_width, int in_height, int out_width, int out_height, int bitrate) {
  // set memory for the decoder
	decoder_t *decoder = (decoder_t *) malloc(sizeof(decoder_t));
	memset(decoder, 0, sizeof(decoder_t));

  // init ffmpeg decoder for H264 software encoding
	avcodec_register_all();
	decoder->codec = avcodec_find_decoder(AV_CODEC_ID_H264);

  // allocate and set ffmpeg context
	decoder->context = avcodec_alloc_context3(decoder->codec);

  // open capture decoder
	avcodec_open2(decoder->context, decoder->codec, NULL);

  // alloc frame format
	decoder->frame = (AVFrame *) av_frame_alloc();
	decoder->frame->format = AV_PIX_FMT_YUV420P;
	decoder->frame->width  = in_width;
	decoder->frame->height = in_height;
	decoder->frame->pts = 0;

	return decoder;
}

/// @brief destroy decoder decoder
/// @details frees FFmpeg decoder memory

void destroy_video_decoder(decoder_t *decoder) {
  // check if decoder decoder exists
	if (decoder == NULL) {
    	printf("Cannot destroy decoder decoder.\n");
    	return;
  	}

  // free the ffmpeg contextes
	avcodec_close(decoder->context);

  // free the decoder context and frame
	av_free(decoder->context);
	av_free(decoder->frame);

  // free the buffer and decoder
	free(decoder);
	return;
}

/// @brief decode a frame using the decoder decoder
/// @details decode an encoded frame under YUV color format into RGB frame
void *video_decoder_decode(decoder_t *decoder, char *buffer, int buffer_size) {
  // init packet to prepare decoding
	av_init_packet(&decoder->packet);
	int success = 0; // boolean for success or failure of decoding

  // copy the received packet back into the decoder AVPacket
  // memcpy(&decoder->packet.data, &buffer, buffer_size);
	decoder->packet.data = buffer;
	decoder->packet.size = buffer_size;
  // decode the frame
	avcodec_decode_video2(decoder->context, decoder->frame, &success, &decoder->packet);
	av_free_packet(&decoder->packet);

    return;
}
