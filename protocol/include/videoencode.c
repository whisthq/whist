#include <stdio.h>
#include <stdlib.h>

#include "videoencode.h"  // header file for this file

static AVBufferRef *hw_device_ctx = NULL;

void set_opt(encoder_t *encoder, char *option, char *value) {
    int ret = av_opt_set(encoder->context->priv_data, option, value, 0);
    if (ret < 0) {
        mprintf("Could not av_opt_set %s to %s!", option, value);
    }
}

int try_setup_video_encoder(encoder_t *encoder, int bitrate, int gop_size) {
    if (encoder->type == NVENC_ENCODE) {
        mprintf( "Trying Nvidia encoder\n" );

        enum AVPixelFormat out_format = AV_PIX_FMT_0RGB32;

        clock t;
        StartTimer(&t);
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA,
                                   "CUDA", NULL, 0) < 0) {
            mprintf("Failed to create specified device.\n");
            return -1;
        }
        mprintf("Time to create encoder: %f\n", GetTimer(t));
        encoder->codec = avcodec_find_encoder_by_name("h264_nvenc");

        enum AVPixelFormat encoder_format = AV_PIX_FMT_CUDA;

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->width;
        encoder->context->height = encoder->height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = bitrate;
        encoder->context->rc_buffer_size = 8 * (bitrate / 8 / 30);
        encoder->context->gop_size = gop_size;
        encoder->context->keyint_min = 5;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = 30;
        encoder->context->pix_fmt = encoder_format;

        set_opt(encoder, "nonref_p", "1");
        set_opt(encoder, "preset", "llhp");
        set_opt(encoder, "rc", "cbr_ld_hq");
        set_opt(encoder, "zerolatency", "1");
        set_opt(encoder, "delay", "0");

        av_buffer_unref(&encoder->context->hw_frames_ctx);
        encoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);

        AVHWFramesContext *frames_ctx;

        frames_ctx = (AVHWFramesContext *)encoder->context->hw_frames_ctx->data;
        frames_ctx->format = encoder_format;
        frames_ctx->sw_format = out_format;
        frames_ctx->width = encoder->context->width;
        frames_ctx->height = encoder->context->height;

        av_hwframe_ctx_init(encoder->context->hw_frames_ctx);

        if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
            mprintf("Failed to open video encoder context\n");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->width;
        encoder->sw_frame->height = encoder->height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = avpicture_get_size(out_format, encoder->width,
                                            encoder->height);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        avpicture_fill((AVPicture *)encoder->sw_frame,
                       (uint8_t *)encoder->frame_buffer, out_format,
                       encoder->width, encoder->height);

        // set sws context for color format conversion
        encoder->sws = NULL;

        encoder->hw_frame = av_frame_alloc();
        if (av_hwframe_get_buffer(encoder->context->hw_frames_ctx,
                                  encoder->hw_frame, 0) < 0) {
            mprintf("Failed to init buffer for video encoder hw frames\n");
            return -1;
        }
    } else if (encoder->type == QSV_ENCODE) {
        mprintf("Trying QSV encoder\n");

        enum AVPixelFormat out_format = AV_PIX_FMT_0RGB32;

        clock t;
        StartTimer(&t);
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_QSV, NULL,
                                   NULL, 0) < 0) {
            mprintf("Failed to create specified device.\n");
            return -1;
        }
        mprintf("Time to create encoder: %f\n", GetTimer(t));
        encoder->codec = avcodec_find_encoder_by_name("h264_qsv");

        enum AVPixelFormat encoder_format = AV_PIX_FMT_QSV;

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->width;
        encoder->context->height = encoder->height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = bitrate;
        encoder->context->gop_size = gop_size;
        encoder->context->keyint_min = 5;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = 30;
        encoder->context->pix_fmt = encoder_format;

        set_opt(encoder, "nonref_p", "1");
        set_opt(encoder, "preset", "llhp");
        set_opt(encoder, "rc", "cbr_ld_hq");
        set_opt(encoder, "zerolatency", "1");
        set_opt(encoder, "delay", "0");

        av_buffer_unref(&encoder->context->hw_frames_ctx);
        encoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);

        AVHWFramesContext *frames_ctx;

        frames_ctx = (AVHWFramesContext *)encoder->context->hw_frames_ctx->data;
        frames_ctx->format = encoder_format;
        frames_ctx->sw_format = out_format;
        frames_ctx->width = encoder->context->width;
        frames_ctx->height = encoder->context->height;

        av_hwframe_ctx_init(encoder->context->hw_frames_ctx);

        if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
            mprintf("Failed to open video encoder context\n");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->width;
        encoder->sw_frame->height = encoder->height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = avpicture_get_size(out_format, encoder->width,
                                            encoder->height);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        avpicture_fill((AVPicture *)encoder->sw_frame,
                       (uint8_t *)encoder->frame_buffer, out_format,
                       encoder->width, encoder->height);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(encoder->width, encoder->height,
                                      AV_PIX_FMT_RGB32, encoder->width,
                                      encoder->height, out_format,
                                      SWS_BILINEAR, 0, 0, 0);
        if (!encoder->sws) {
            mprintf("Failed to initialize swsContext for video encoder\n");
            return -1;
        }
        encoder->hw_frame = av_frame_alloc();
        if (av_hwframe_get_buffer(encoder->context->hw_frames_ctx,
                                  encoder->hw_frame, 0) < 0) {
            mprintf("Failed to init buffer for video encoder hw frames\n");
            return -1;
        }
    } else if (encoder->type == SOFTWARE_ENCODE) {
        mprintf("Trying software encoder\n");

        enum AVPixelFormat out_format = AV_PIX_FMT_YUV420P;

        encoder->codec = avcodec_find_encoder(AV_CODEC_ID_H264);

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->width;
        encoder->context->height = encoder->height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = bitrate;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = 30;
        encoder->context->gop_size = gop_size;
        encoder->context->pix_fmt = out_format;

        set_opt( encoder, "nonref_p", "1" );
        set_opt( encoder, "preset", "llhp" );
        set_opt( encoder, "rc", "cbr_ld_hq" );
        set_opt( encoder, "zerolatency", "1" );
        set_opt( encoder, "delay", "0" );

        if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
            mprintf("Failed to open context for stream\n");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->width;
        encoder->sw_frame->height = encoder->height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = avpicture_get_size(
            out_format, encoder->width, encoder->height);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        avpicture_fill((AVPicture *)encoder->sw_frame,
                       (uint8_t *)encoder->frame_buffer, out_format,
                       encoder->width, encoder->height);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(encoder->width, encoder->height,
                                      AV_PIX_FMT_RGB32, encoder->width,
                                      encoder->height, out_format,
                                      SWS_BICUBIC, 0, 0, 0);
        if (!encoder->sws) {
            mprintf("Failed to initialize swsContext for video encoder\n");
            return -1;
        }
    } else {
        mprintf("Unsupported hardware type!\n");
        return -1;
    }

    mprintf( "Video Encoder created!\n" );
    return 0;
}

/// @brief creates encoder encoder
/// @details creates FFmpeg encoder
encoder_t *create_video_encoder(int width, int height, int bitrate, int gop_size) {
    // set memory for the encoder
    encoder_t *encoder = (encoder_t *)malloc(sizeof(encoder_t));
    memset(encoder, 0, sizeof(encoder_t));

    // get frame dimensions for encoder
    encoder->width = width;
    encoder->height = height;

    int encoder_precedence[] = { NVENC_ENCODE, QSV_ENCODE, SOFTWARE_ENCODE};

    for (unsigned long i = 0;
         i < sizeof(encoder_precedence) / sizeof(encoder_precedence[0]); ++i) {
        encoder->type = encoder_precedence[i];
        if (try_setup_video_encoder(encoder, bitrate, gop_size) < 0) {
            mprintf("Video encoder: Failed, trying next encoder\n");
        } else {
            mprintf("Video encoder: Success!\n");
            return encoder;
        }
    }

    mprintf("Video encoder: All encoders failed!\n");
    return NULL;
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
    if( encoder->sws )
    {
        sws_freeContext( encoder->sws );
    }
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
void video_encoder_encode(encoder_t *encoder, void *rgb_pixels) {
    // init packet to prepare encoding
    av_packet_unref( &encoder->packet );
    av_init_packet( &encoder->packet );

    if( encoder->sws )
    {
        uint8_t* in_data[1] = { (uint8_t*)rgb_pixels };
        int in_linesize[1] = { encoder->width * 4 };

        // convert to the encoder format
        sws_scale( encoder->sws, in_data, in_linesize, 0, encoder->height,
                   encoder->sw_frame->data, encoder->sw_frame->linesize );
    } else
    {
        memset( encoder->sw_frame->data, 0, sizeof( encoder->sw_frame->data ) );
        memset( encoder->sw_frame->linesize, 0,
                sizeof( encoder->sw_frame->linesize ) );
        encoder->sw_frame->data[0] = (uint8_t*)rgb_pixels;
        encoder->sw_frame->linesize[0] = encoder->width * 4;
    }

    int success = 0;  // boolean for success or failure of encoding

    // define input data to encoder
    if (encoder->type == NVENC_ENCODE || encoder->type == QSV_ENCODE) {
        av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);
        avcodec_encode_video2(encoder->context, &encoder->packet,
                              encoder->hw_frame, &success);
    } else if (encoder->type == SOFTWARE_ENCODE) {
        avcodec_encode_video2(encoder->context, &encoder->packet,
                              encoder->sw_frame, &success);
    } else {
        mprintf("Invalid encoder type\n");
    }
}
