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
static enum AVPixelFormat hw_pix_fmt;
static AVBufferRef *hw_device_ctx = NULL;
/// @brief creates decoder decoder
/// @details creates FFmpeg decoder

void swap_decoder(void *ptr, int level, const char *fmt, va_list vargs)
{
  mprintf("Error found\n");
  vprintf(fmt, vargs);
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        mprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

static enum AVPixelFormat get_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    mprintf(stderr, "Failed to get HW surface format.\n");
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

    hw_pix_fmt = AV_PIX_FMT_QSV;

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
  } else {
     // set the appropriate video decoder format based on PS
      #if defined(_WIN32)
        hw_pix_fmt = AV_PIX_FMT_D3D11VA_VLD;
        char *device_type = "d3d11va";
      #elif __APPLE__
        hw_pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
        char *device_type = "videotoolbox";
      #else // linux
        hw_pix_fmt = AV_PIX_FMT_VAAPI;
        char *device_type = "vaapi";
      #endif

      // get the appropriate hardware device
      decoder->device_type = av_hwdevice_find_type_by_name(device_type);
      if (decoder->device_type == AV_HWDEVICE_TYPE_NONE) {
          while((decoder->device_type = av_hwdevice_iterate_types(decoder->device_type)) != AV_HWDEVICE_TYPE_NONE)
              mprintf(" %s", av_hwdevice_get_type_name(decoder->device_type));
          return decoder;
      }

      decoder->codec = avcodec_find_decoder(AV_CODEC_ID_H264);

      for (int i = 0;; i++) {
          const AVCodecHWConfig *config = avcodec_get_hw_config(decoder->codec, i);
          if (!config) {
              mprintf("Decoder %s does not support device type %s.\n",
                      decoder->codec->name, av_hwdevice_get_type_name(decoder->device_type));
              return decoder;
          }
          if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
              config->device_type == decoder->device_type) {
              hw_pix_fmt = config->pix_fmt;
              break;
          }
      }

      if (!(decoder->context = avcodec_alloc_context3(decoder->codec))) {
        mprintf("alloccontext3 failed w/ error code: %d\n", AVERROR(ENOMEM));
        return decoder;
      }

        decoder->context->get_format = get_format;
        av_opt_set(decoder->context->priv_data, "async_depth", "1", 0);

      if (hw_decoder_init(decoder->context, decoder->device_type) < 0) {
        mprintf("hwdecoder_init failed\n");
        return decoder;
      }

      if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
          mprintf("Failed to open codec for stream\n");
          return decoder;
      }

      if (!(decoder->hw_frame = av_frame_alloc()) || !(decoder->sw_frame = av_frame_alloc())) {
          mprintf("Can not alloc frame\n");

          av_frame_free(&decoder->hw_frame);
          av_frame_free(&decoder->sw_frame);
      }
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

  /* flush the decoder */
  decoder->packet.data = NULL;
  decoder->packet.size = 0;
  av_packet_unref(&decoder->packet);
  avcodec_free_context(&decoder->context);

  // unref the hw context
  av_buffer_unref(&hw_device_ctx);

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
  } else {
    decoder->packet.data = buffer;
    decoder->packet.size = buffer_size;

    if(avcodec_send_packet(decoder->context, &decoder->packet) < 0) {
      return NULL;
    }

    if(avcodec_receive_frame(decoder->context, decoder->hw_frame) < 0) {
      return NULL;
    }

    if (decoder->hw_frame->format == hw_pix_fmt) {
        if (av_hwframe_transfer_data(decoder->sw_frame, decoder->hw_frame, 0) < 0) {
            return NULL;
        }
    }
  }
}
