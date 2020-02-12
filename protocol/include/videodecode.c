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


extern volatile DecodeType type = DECODE_TYPE_NONE;
static AVBufferRef *hw_device_ctx = NULL;
/// @brief creates decoder decoder
/// @details creates FFmpeg decoder

void swap_decoder(void *ptr, int level, const char *fmt, va_list vargs)
{
  mprintf("Error found\n");
  vprintf(fmt, vargs);
}

static enum AVPixelFormat get_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
      // for apple devices, we use videotoolbox, else we use QSV
      #if defined(__APPLE__)
        if (*p == AV_PIX_FMT_VIDEOTOOLBOX) {
          mprintf("Videotoolbox found\n");
          return *p;
        }
      // QSV for windows and linux
      #else
        if (*p == AV_PIX_FMT_QSV) {
           //mprintf("QSV found\n");
           return *p;
        }
      #endif
    }
    mprintf("Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

void set_decoder(bool hardware) {
  if(hardware) {
    #if defined(_WIN32)
      type = DECODE_TYPE_QSV;
    #elif __APPLE__
      type = DECODE_TYPE_VIDEOTOOLBOX;
    #else // linux
      type = DECODE_TYPE_VAAPI;
    #endif
  } else {
    type = DECODE_TYPE_SOFTWARE;
  }
}

video_decoder_t*create_video_decoder(int in_width, int in_height, int out_width, int out_height, DecodeType type) {
  video_decoder_t*decoder = (video_decoder_t*) malloc(sizeof(video_decoder_t));
  memset(decoder, 0, sizeof(video_decoder_t));

  decoder->type = type;
  // init ffmpeg decoder for H264 software encoding
  avcodec_register_all();

  if(type == DECODE_TYPE_SOFTWARE) {
    mprintf("Using software decoder\n");
    decoder->codec = avcodec_find_decoder_by_name("h264");
    decoder->context = avcodec_alloc_context3(decoder->codec);

    avcodec_open2(decoder->context, decoder->codec, NULL);

    decoder->sw_frame = (AVFrame *) av_frame_alloc();
    decoder->sw_frame->format = AV_PIX_FMT_YUV420P;
    decoder->sw_frame->width  = in_width;
    decoder->sw_frame->height = in_height;
    decoder->sw_frame->pts = 0;
  } else if(type == DECODE_TYPE_QSV) {
    mprintf("Using QSV decoder\n");
    decoder->codec = avcodec_find_decoder_by_name("h264_qsv");
    decoder->context = avcodec_alloc_context3(decoder->codec);
    decoder->context->get_format  = get_format;

    av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_QSV, "auto", NULL, 0);
    decoder->context->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    av_buffer_unref(&decoder->context->hw_frames_ctx);
    decoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);

    AVHWFramesContext *frames_ctx;
    AVQSVFramesContext *frames_hwctx;

    frames_ctx = (AVHWFramesContext*)decoder->context->hw_frames_ctx->data;
    frames_hwctx = frames_ctx->hwctx;

    frames_ctx->format = AV_PIX_FMT_QSV;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = in_width;
    frames_ctx->height = in_height;
    frames_ctx->initial_pool_size = 32;
    frames_hwctx->frame_type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;


    av_hwframe_ctx_init(decoder->context->hw_frames_ctx);
    av_opt_set(decoder->context->priv_data, "async_depth", "1", 0);

    avcodec_open2(decoder->context, NULL, NULL);

    decoder->sw_frame = av_frame_alloc();
    decoder->hw_frame = av_frame_alloc();
    av_hwframe_get_buffer(decoder->context->hw_frames_ctx, decoder->hw_frame, 0);
  }

  return decoder;
}

/// @brief destroy decoder decoder
/// @details frees FFmpeg decoder memory

void destroy_video_decoder(video_decoder_t*decoder) {
  // check if decoder decoder exists
  if (decoder == NULL) {
      mprintf("Cannot destroy decoder decoder.\n");
      return;
    }

  // free the ffmpeg contextes
  avcodec_close(decoder->context);

  // free the decoder context and frame
  av_free(decoder->context);
  av_free(decoder->sw_frame);
  av_free(decoder->hw_frame);

  // free the buffer and decoder
  free(decoder);
  return;
}

/// @brief decode a frame using the decoder decoder
/// @details decode an encoded frame under YUV color format into RGB frame
void *video_decoder_decode(video_decoder_t*decoder, void *buffer, int buffer_size) {
  // init packet to prepare decoding
  // av_log_set_level(AV_LOG_ERROR);
  // av_log_set_callback(swap_decoder);
  if(type == DECODE_TYPE_QSV || type == DECODE_TYPE_SOFTWARE) {
    av_init_packet(&decoder->packet);
    int success = 0, ret = 0; // boolean for success or failure of decoding

    // copy the received packet back into the decoder AVPacket
    // memcpy(&decoder->packet.data, &buffer, buffer_size);
    decoder->packet.data = buffer;
    decoder->packet.size = buffer_size;
    // decode the frame
    ret = avcodec_decode_video2(decoder->context, decoder->sw_frame, &success, &decoder->packet);
    
    // av_hwframe_transfer_data(decoder->sw_frame, decoder->hw_frame, 0);

    av_free_packet(&decoder->packet);
    return NULL;
  }
}
