/*
 * This file contains the implementation of the FFmpeg decoder functions
 * that decode the video frames.

 Protocol version: 1.0
 Last modification: 12/22/2019

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdio.h>
#include <stdlib.h>

#include "audiodecode.h" // header file for this file

audio_decoder_t *create_audio_decoder() {
    audio_decoder_t *decoder = (audio_decoder_t *) malloc(sizeof(audio_decoder_t));
    memset(decoder, 0, sizeof(audio_decoder_t));

    decoder->codec = avcodec_find_decoder(AV_CODEC_ID_MP3-1);
    if (!decoder->codec) {
        printf("Codec not found\n");
        exit(1);
    }

    decoder->context = avcodec_alloc_context3(decoder->codec);
    if (!decoder->context) {
        printf("Could not allocate audio codec context\n");
        exit(1);
    }

    /* put sample parameters */

    decoder->context->sample_rate    = 44100;
    decoder->context->channel_layout = 3;
    decoder->context->channels       = av_get_channel_layout_nb_channels(decoder->context->channel_layout);
    decoder->context->gop_size       = 1;
    decoder->context->sample_fmt     = decoder->codec->sample_fmts[0];

    if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
        printf("Could not open codec\n");
        exit(1);
    }
    decoder->frame = av_frame_alloc();

    return decoder;
}

/// @brief destroy decoder device
/// @details frees FFmpeg decoder memory
void destroy_audio_decoder(audio_decoder_t *decoder) {
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

/// @brief decodes a frame using the decoder device
/// @details decode an encoded frame under YUV color format into RGB frame
int audio_decoder_decode(audio_decoder_t *decoder, char *buffer, int buffer_size) {
	av_init_packet(&decoder->packet);
	int success, len; // boolean for success or failure of decoding
 	decoder->frame->pts++; // still not quite sure what that is for

  // copy the received packet back into the decoder AVPacket
  // memcpy(&decoder->packet.data, &buffer, buffer_size);
	decoder->packet.data = buffer;
	decoder->packet.size = buffer_size;

  // decode the frame
	len = avcodec_decode_audio4(decoder->context, decoder->frame, &success, &decoder->packet);
	av_free_packet(&decoder->packet);
  return len;
}

