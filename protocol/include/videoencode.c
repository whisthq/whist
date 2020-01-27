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

static AVBufferRef *hw_device_ctx = NULL;

/// @brief creates encoder encoder
/// @details creates FFmpeg encoder
encoder_t *create_video_encoder(int in_width, int in_height, int out_width, int out_height, int bitrate, EncodeType type) {
  // set memory for the encoder
	encoder_t *encoder = (encoder_t *) malloc(sizeof(encoder_t));
	memset(encoder, 0, sizeof(encoder_t));

  // get frame dimensions for encoder
	encoder->in_width = in_width;
	encoder->in_height = in_height;
	encoder->out_width = out_width;
	encoder->out_height = out_height;
	encoder->type = type;

	avcodec_register_all();

	if(type == NVENC_ENCODE) {
    	av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, "CUDA", NULL, 0);
   		encoder->codec = avcodec_find_encoder_by_name("h264_nvenc");

		encoder->context = avcodec_alloc_context3(encoder->codec);
		encoder->context->width     = out_width;
		encoder->context->height    = out_height;
		encoder->context->bit_rate  = bitrate;
		encoder->context->time_base.num = 1;
		encoder->context->time_base.den = 30;
		encoder->context->gop_size = 10;
		encoder->context->pix_fmt   = AV_PIX_FMT_CUDA;

		av_opt_set(encoder->context->priv_data, "preset", "llhp", 0);
		av_opt_set(encoder->context->priv_data, "zerolatency", "1", 0);
		av_opt_set(encoder->context->priv_data, "async_depth", "1", 0);
		av_opt_set(encoder->context->priv_data, "delay", "0", 0);

		av_buffer_unref(&encoder->context->hw_frames_ctx);
		encoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);

		AVHWFramesContext *frames_ctx;

		frames_ctx = (AVHWFramesContext*)encoder->context->hw_frames_ctx->data;
		frames_ctx->format = AV_PIX_FMT_CUDA;
		frames_ctx->sw_format = AV_PIX_FMT_YUV420P;
		frames_ctx->width = encoder->context->width;
		frames_ctx->height = encoder->context->height;

		av_hwframe_ctx_init(encoder->context->hw_frames_ctx);

		avcodec_open2(encoder->context, encoder->codec, NULL);

		encoder->sw_frame = (AVFrame *) av_frame_alloc();
		encoder->sw_frame->format = AV_PIX_FMT_YUV420P;
		encoder->sw_frame->width  = out_width;
		encoder->sw_frame->height = out_height;
		encoder->sw_frame->pts = 0;

	// set frame size and allocate memory for it
		int frame_size = avpicture_get_size(AV_PIX_FMT_YUV420P, out_width, out_height);
		encoder->frame_buffer = malloc(frame_size);

	// fill picture with empty frame buffer
		avpicture_fill((AVPicture *) encoder->sw_frame, (uint8_t *) encoder->frame_buffer, AV_PIX_FMT_YUV420P, out_width, out_height);

	// set sws context for color format conversion
		encoder->sws = sws_getContext(in_width, in_height, AV_PIX_FMT_RGB32, out_width, out_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

		encoder->hw_frame = av_frame_alloc();
		av_hwframe_get_buffer(encoder->context->hw_frames_ctx, encoder->hw_frame, 0);
	} else {
   		encoder->codec = avcodec_find_encoder_by_name("libx264");

		encoder->context = avcodec_alloc_context3(encoder->codec);
		encoder->context->width     = out_width;
		encoder->context->height    = out_height;
		encoder->context->bit_rate  = bitrate;
		encoder->context->time_base.num = 1;
		encoder->context->time_base.den = 30;
		encoder->context->gop_size = 1;
		encoder->context->pix_fmt   = AV_PIX_FMT_YUV420P;

		av_opt_set(encoder->context->priv_data, "preset", "ultrafast", 0);
		av_opt_set(encoder->context->priv_data, "tune", "zerolatency", 0);

		avcodec_open2(encoder->context, encoder->codec, NULL);

		encoder->sw_frame = (AVFrame *) av_frame_alloc();
		encoder->sw_frame->format = AV_PIX_FMT_YUV420P;
		encoder->sw_frame->width  = out_width;
		encoder->sw_frame->height = out_height;
		encoder->sw_frame->pts = 0;

	// set frame size and allocate memory for it
		int frame_size = avpicture_get_size(AV_PIX_FMT_YUV420P, out_width, out_height);
		encoder->frame_buffer = malloc(frame_size);

	// fill picture with empty frame buffer
		avpicture_fill((AVPicture *) encoder->sw_frame, (uint8_t *) encoder->frame_buffer, AV_PIX_FMT_YUV420P, out_width, out_height);

	// set sws context for color format conversion
		encoder->sws = sws_getContext(in_width, in_height, AV_PIX_FMT_RGB32, out_width, out_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
	}

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
	av_free(encoder->sw_frame);
	av_free(encoder->hw_frame);

  // free the buffer and encoder
	free(encoder->frame_buffer);
	free(encoder);
	return;
}

/// @brief encode a frame using the encoder encoder
/// @details encode a RGB frame into encoded format as YUV color
void *video_encoder_encode(encoder_t *encoder, void *rgb_pixels) {
  // define input data to encoder
	uint8_t *in_data[1] = {(uint8_t *) rgb_pixels};
	int in_linesize[1] = {encoder->in_width * 4};
	
    sws_scale(encoder->sws, in_data, in_linesize, 0, encoder->in_height, encoder->sw_frame->data, encoder->sw_frame->linesize);
        
  // init packet to prepare encoding
	av_free_packet(&encoder->packet);
	av_init_packet(&encoder->packet);
	int success = 0; // boolean for success or failure of encoding

  // attempt to encode the frame
	if(encoder->type == NVENC_ENCODE) {
    	av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);
		avcodec_encode_video2(encoder->context, &encoder->packet, encoder->hw_frame, &success);
	} else {
		avcodec_encode_video2(encoder->context, &encoder->packet, encoder->sw_frame, &success);
	}
}
