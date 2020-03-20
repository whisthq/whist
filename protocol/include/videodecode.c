#if defined(_WIN32)
#pragma warning(disable: 4706) // assignment within conditional warning
#endif 

#include <stdio.h>
#include <stdlib.h>

#include "videodecode.h" // header file for this file

static enum AVPixelFormat hw_pix_fmt;
static AVBufferRef *hw_device_ctx = NULL;

void swap_decoder(const char *fmt, va_list vargs)
{
  mprintf("Error found\n");
  vprintf(fmt, vargs);
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        mprintf((const char *) stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat *pix_fmts) {
    ctx;

    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if( *p == hw_pix_fmt )
        {
            mprintf( "Hardware format found\n" );
            return *p;
        }
    }

    mprintf("Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

video_decoder_t* create_video_decoder(int width, int height, bool use_hardware) {
  video_decoder_t* decoder = (video_decoder_t*) malloc(sizeof(video_decoder_t));
  memset(decoder, 0, sizeof(video_decoder_t));

  if (use_hardware) {
    #if defined(_WIN32)
      decoder->type = DECODE_TYPE_D3D11;
    #elif __APPLE__
      decoder->type = DECODE_TYPE_VIDEOTOOLBOX;
    #else // linux
      decoder->type = DECODE_TYPE_VAAPI;
    #endif
  } else {
    decoder->type = DECODE_TYPE_SOFTWARE;
  }

  if(decoder->type == DECODE_TYPE_SOFTWARE) {
    mprintf("Using software decoder\n");
    decoder->codec = avcodec_find_decoder_by_name("h264");
    if (!decoder->codec) {
      mprintf("Could not find video codec\n");
      return NULL;
    }
    decoder->context = avcodec_alloc_context3(decoder->codec);

    avcodec_open2(decoder->context, decoder->codec, NULL);

    decoder->sw_frame = (AVFrame *) av_frame_alloc();
    decoder->sw_frame->format = AV_PIX_FMT_YUV420P;
    decoder->sw_frame->width  = width;
    decoder->sw_frame->height = height;
    decoder->sw_frame->pts = 0;
  } else if(decoder->type == DECODE_TYPE_QSV) {
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
    frames_ctx->width = width;
    frames_ctx->height = height;
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


    int ret = 0;

    // get the appropriate hardware device
    decoder->device_type = av_hwdevice_find_type_by_name(device_type);
    if (decoder->device_type == AV_HWDEVICE_TYPE_NONE) {
        mprintf("Device type %s is not supported.\n", device_type);
        mprintf("Available device types:");
        while((decoder->device_type = av_hwdevice_iterate_types(decoder->device_type)) != AV_HWDEVICE_TYPE_NONE)
            mprintf(" %s", av_hwdevice_get_type_name(decoder->device_type));
        mprintf("\n");
        return decoder;
    }

    decoder->codec = avcodec_find_decoder(AV_CODEC_ID_H264);

    // get the hw config
    for (int i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder->codec, i);
        if (!config) {
            mprintf("Decoder %s does not support device type %s.\n",
                    decoder->codec->name, av_hwdevice_get_type_name(decoder->device_type));
            return NULL;
        }
        if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
            config->device_type == decoder->device_type) {
            hw_pix_fmt = config->pix_fmt;
            mprintf( "Suppoted format: %d %s\n", config->pix_fmt, av_get_pix_fmt_name(config->pix_fmt));
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

    if ((ret = avcodec_open2(decoder->context, decoder->codec, NULL)) < 0) {
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
bool video_decoder_decode(video_decoder_t*decoder, void *buffer, int buffer_size) {
    static double total_time = 0.0;
    static int num_times = 0;

    clock t;
    StartTimer( &t );

  // init packet to prepare decoding
  // av_log_set_level(AV_LOG_ERROR);
  // av_log_set_callback(swap_decoder);
  av_init_packet( &decoder->packet );

  // copy the received packet back into the decoder AVPacket
  // memcpy(&decoder->packet.data, &buffer, buffer_size);
  decoder->packet.data = buffer;
  decoder->packet.size = buffer_size;
  // decode the frame
  if( avcodec_send_packet( decoder->context, &decoder->packet ) < 0 )
  {
      mprintf( "Failed to avcodec_send_packet!\n" );
      return false;
  }

  // If frame was computed on the CPU
  if(decoder->type == DECODE_TYPE_QSV || decoder->type == DECODE_TYPE_SOFTWARE) {
    if(avcodec_receive_frame(decoder->context, decoder->sw_frame) < 0) {
      mprintf( "Failed to avcodec_receive_frame!\n" );
      return false;
    }

    // av_hwframe_transfer_data(decoder->sw_frame, decoder->hw_frame, 0);

  } else {
  // If frame was computed on the GPU
    if(avcodec_receive_frame(decoder->context, decoder->hw_frame) < 0) {
        mprintf( "Failed to avcodec_receive_frame!\n" );
      return false;
    }

    if (decoder->hw_frame->format == hw_pix_fmt) {
        if (av_hwframe_transfer_data(decoder->sw_frame, decoder->hw_frame, 0) < 0) {
            mprintf( "Failed to av_hwframe_transfer_data!\n" );
            return false;
        }
    } else
    {
        mprintf( "Incorrect hw frame format!\n" );
        return false;
    }
  }

  av_packet_unref(&decoder->packet);

  double time = GetTimer( t );
  //mprintf( "Decode Time: %f\n", time );
  if( time < 0.020 )
  {
      total_time += time;
      num_times++;
  //    mprintf( "Avg Decode Time: %f\n", total_time / num_times );
  }

  return true;
}

#if defined(_WIN32)
#pragma warning(default: 4706)
#endif 