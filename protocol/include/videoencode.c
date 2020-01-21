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

static int set_hwframe_ctx(AVCodecContext *ctx, AVBufferRef *hw_device_ctx)
{
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create CUDA frame context.\n");
        return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = AV_PIX_FMT_YUV420P;
    frames_ctx->width     = 1280;
    frames_ctx->height    = 720;
    frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        fprintf(stderr, "Failed to initialize CUDA frame context."
                "Error code: %s\n",av_err2str(err));
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}
/// @brief creates encoder encoder
/// @details creates FFmpeg encoder
encoder_t *create_video_encoder(int in_width, int in_height, int out_width, int out_height, int bitrate) {
  // set memory for the encoder
	int ret;
	encoder_t *encoder = (encoder_t *) malloc(sizeof(encoder_t));
	memset(encoder, 0, sizeof(encoder_t));

  // get frame dimensions for encoder
	encoder->in_width = in_width;
	encoder->in_height = in_height;
	encoder->out_width = out_width;
	encoder->out_height = out_height;

  // init ffmpeg encoder for H264 software encoding
	avcodec_register_all();

    /* find the video encoder */
    int size, err;


    err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA,
                                 "CUDA", NULL, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to create a CUDA device. Error code: %s\n", av_err2str(err));
        
    }

    if (!(encoder->codec = avcodec_find_encoder_by_name("h264_nvenc"))) {
        fprintf(stderr, "Could not find encoder.\n");
        err = -1;
        
    }

    if (!(encoder->context = avcodec_alloc_context3(encoder->codec))) {
        err = AVERROR(ENOMEM);
        printf("Could not allocate context\n");
    }

    encoder->context->width     = out_width;
    encoder->context->height    = out_height;
    encoder->context->bit_rate  = bitrate;
	encoder->context->time_base.num = 1;
	encoder->context->time_base.den = 30;
    encoder->context->gop_size = 1;
    encoder->context->pix_fmt   = AV_PIX_FMT_CUDA;

    av_opt_set(encoder->context->priv_data, "tune", "zerolatency", 0);
	av_opt_set(encoder->context->priv_data, "preset", "llhp", 0);


    av_buffer_unref(&encoder->context->hw_frames_ctx);
    encoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
    if (!encoder->context->hw_frames_ctx) {
        av_log(encoder->context, AV_LOG_ERROR, "Error creating a CUDA frames context\n");
        return AVERROR(ENOMEM);
    }

    AVHWFramesContext *frames_ctx;
    frames_ctx = (AVHWFramesContext*)encoder->context->hw_frames_ctx->data;

    frames_ctx->format = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = AV_PIX_FMT_YUV420P;
    frames_ctx->width = encoder->context->width;
    frames_ctx->height = encoder->context->height;

    ret = av_hwframe_ctx_init(encoder->context->hw_frames_ctx);
    if (ret < 0) {
        av_log(encoder->context, AV_LOG_ERROR, "Error initializing a CUDA frame pool\n");
    }


    if ((err = avcodec_open2(encoder->context, encoder->codec, NULL)) < 0) {
        fprintf(stderr, "Cannot open video encoder codec. Error code: %s\n", av_err2str(err));
    }

  // alloc frame format
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

    if (!(encoder->hw_frame = av_frame_alloc())) {
    	printf("Could not create hardware frame\n");
        err = AVERROR(ENOMEM);
        
    }
    if ((err = av_hwframe_get_buffer(encoder->context->hw_frames_ctx, encoder->hw_frame, 0)) < 0) {
        fprintf(stderr, "Error code: %s.\n", av_err2str(err));
    }
    if (!encoder->hw_frame->hw_frames_ctx) {
    	printf("No hardware frame context\n");
        err = AVERROR(ENOMEM);
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
	int err;
	uint8_t *in_data[1] = {(uint8_t *) rgb_pixels};
	int in_linesize[1] = {encoder->in_width * 4};
	
    sws_scale(encoder->sws, in_data, in_linesize, 0, encoder->in_height, encoder->sw_frame->data, encoder->sw_frame->linesize);
    
    if ((err = av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0)) < 0) {
        fprintf(stderr, "Error while transferring frame data to surface."
                "Error code: %s.\n", av_err2str(err));
        
    }
    
  // init packet to prepare encoding
	av_free_packet(&encoder->packet);
	av_init_packet(&encoder->packet);
	int success = 0; // boolean for success or failure of encoding

  // attempt to encode the frame
	avcodec_encode_video2(encoder->context, &encoder->packet, encoder->hw_frame, &success);

    // av_frame_free(&encoder->hw_frame);
}
