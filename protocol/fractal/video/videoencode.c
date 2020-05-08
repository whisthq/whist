/*
 * Video encoding via FFmpeg library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "videoencode.h"

#include <stdio.h>
#include <stdlib.h>

void set_opt(encoder_t *encoder, char *option, char *value) {
    int ret = av_opt_set(encoder->context->priv_data, option, value, 0);
    if (ret < 0) {
        LOG_WARNING("Could not av_opt_set %s to %s!", option, value);
    }
}

int try_setup_video_encoder(encoder_t *encoder, int bitrate, int gop_size) {
    // setup the AVCodec and AVFormatContext
    // avcodec_register_all is deprecated on FFmpeg 4+
    // only linux uses FFmpeg 3.4.x because of canonical system packages
#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
#endif
    int max_buffer = 4 * (bitrate / FPS);
    AVBufferRef *hw_device_ctx = NULL;

    // TODO: If we end up using graphics card encoding, then we should pass it
    // the image from DXGI WinApi screen capture, so that the uncompressed image
    // doesn't ever hit the CPU or RAM
    if (encoder->type == NVENC_ENCODE) {
        LOG_INFO("Trying Nvidia encoder");

        enum AVPixelFormat out_format = AV_PIX_FMT_0RGB32;

        clock t;
        StartTimer(&t);
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA,
                                   "CUDA", NULL, 0) < 0) {
            LOG_WARNING("Failed to create specified device.");
            return -1;
        }
        LOG_INFO("Time to create encoder: %f\n", GetTimer(t));
        encoder->codec = avcodec_find_encoder_by_name("h264_nvenc");

        enum AVPixelFormat encoder_format = AV_PIX_FMT_CUDA;

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->width;
        encoder->context->height = encoder->height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = bitrate;
        encoder->context->rc_buffer_size = max_buffer;
        encoder->context->gop_size = gop_size;
        encoder->context->keyint_min = 5;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = FPS;
        encoder->context->pix_fmt = encoder_format;

        set_opt(encoder, "nonref_p", "1");
        set_opt(encoder, "preset", "llhp");
        set_opt(encoder, "rc", "cbr_ld_hq");
        set_opt(encoder, "zerolatency", "1");
        set_opt(encoder, "delay", "0");
        // set_opt( encoder, "max-intra-rate", );

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
            LOG_WARNING("Failed to open video encoder context");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->width;
        encoder->sw_frame->height = encoder->height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = av_image_get_buffer_size(out_format, encoder->width,
                                                  encoder->height, 1);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        av_image_fill_arrays(encoder->sw_frame->data,
                             encoder->sw_frame->linesize,
                             (uint8_t *)encoder->frame_buffer, out_format,
                             encoder->width, encoder->height, 1);

        // set sws context for color format conversion
        encoder->sws = NULL;

        encoder->hw_frame = av_frame_alloc();
        if (av_hwframe_get_buffer(encoder->context->hw_frames_ctx,
                                  encoder->hw_frame, 0) < 0) {
            LOG_WARNING("Failed to init buffer for video encoder hw frames");
            return -1;
        }
    } else if (encoder->type == QSV_ENCODE) {
        LOG_INFO("Trying QSV encoder");

        enum AVPixelFormat out_format = AV_PIX_FMT_0RGB32;

        clock t;
        StartTimer(&t);
        if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_QSV, NULL,
                                   NULL, 0) < 0) {
            LOG_WARNING("Failed to create specified device.");
            return -1;
        }
        LOG_INFO("Time to create encoder: %f\n", GetTimer(t));
        encoder->codec = avcodec_find_encoder_by_name("h264_qsv");

        enum AVPixelFormat encoder_format = AV_PIX_FMT_QSV;

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->width;
        encoder->context->height = encoder->height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = bitrate;
        encoder->context->rc_buffer_size = max_buffer;
        encoder->context->gop_size = gop_size;
        encoder->context->keyint_min = 5;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = FPS;
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
            LOG_WARNING("Failed to open video encoder context");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->width;
        encoder->sw_frame->height = encoder->height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = av_image_get_buffer_size(out_format, encoder->width,
                                                  encoder->height, 1);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        av_image_fill_arrays(encoder->sw_frame->data,
                             encoder->sw_frame->linesize,
                             (uint8_t *)encoder->frame_buffer, out_format,
                             encoder->width, encoder->height, 1);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(
            encoder->width, encoder->height, AV_PIX_FMT_RGB32, encoder->width,
            encoder->height, out_format, SWS_BILINEAR, 0, 0, 0);
        if (!encoder->sws) {
            LOG_WARNING("Failed to initialize swsContext for video encoder");
            return -1;
        }
        encoder->hw_frame = av_frame_alloc();
        if (av_hwframe_get_buffer(encoder->context->hw_frames_ctx,
                                  encoder->hw_frame, 0) < 0) {
            LOG_WARNING("Failed to init buffer for video encoder hw frames");
            return -1;
        }
    } else if (encoder->type == SOFTWARE_ENCODE) {
        LOG_INFO("Trying software encoder");

        enum AVPixelFormat out_format = AV_PIX_FMT_YUV420P;

        encoder->codec = avcodec_find_encoder_by_name("libx264");

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->width;
        encoder->context->height = encoder->height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = bitrate;
        encoder->context->rc_buffer_size = max_buffer;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = FPS;
        encoder->context->gop_size = gop_size;
        encoder->context->keyint_min = 5;
        encoder->context->pix_fmt = out_format;
        encoder->context->max_b_frames = 0;

        set_opt(encoder, "nonref_p", "1");
        set_opt(encoder, "preset", "fast");
        set_opt(encoder, "rc", "cbr_ld_hq");
        set_opt(encoder, "zerolatency", "1");
        set_opt(encoder, "tune", "zerolatency");
        set_opt(encoder, "delay", "0");

        if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
            LOG_WARNING("Failed to open context for stream");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->width;
        encoder->sw_frame->height = encoder->height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = av_image_get_buffer_size(out_format, encoder->width,
                                                  encoder->height, 1);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        av_image_fill_arrays(encoder->sw_frame->data,
                             encoder->sw_frame->linesize,
                             (uint8_t *)encoder->frame_buffer, out_format,
                             encoder->width, encoder->height, 1);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(
            encoder->width, encoder->height, AV_PIX_FMT_RGB32, encoder->width,
            encoder->height, out_format, SWS_BICUBIC, 0, 0, 0);

        if (!encoder->sws) {
            LOG_WARNING("Failed to initialize swsContext for video encoder");
            return -1;
        }
    } else {
        LOG_ERROR("Unsupported hardware type!");
        return -1;
    }

    LOG_INFO("Video Encoder created!");
    return 0;
}

// Goes through NVENC/QSV/SOFTWARE and sees which one works, cascading to the
// next one when the previous one doesn't work
encoder_t *create_video_encoder(int width, int height, int bitrate,
                                int gop_size) {
    // set memory for the encoder
    encoder_t *encoder = (encoder_t *)malloc(sizeof(encoder_t));
    memset(encoder, 0, sizeof(encoder_t));

    // get frame dimensions for encoder
    encoder->width = width;
    encoder->height = height;

    int encoder_precedence[] = {NVENC_ENCODE, QSV_ENCODE, SOFTWARE_ENCODE};

    for (unsigned long i = 0;
         i < sizeof(encoder_precedence) / sizeof(encoder_precedence[0]); ++i) {
        encoder->type = encoder_precedence[i];
        if (try_setup_video_encoder(encoder, bitrate, gop_size) < 0) {
            LOG_WARNING(
                "Video encoder: Failed, destroying encoder and trying next "
                "encoder");

            destroy_video_encoder(encoder);
            encoder = (encoder_t *)malloc(sizeof(encoder_t));
            memset(encoder, 0, sizeof(encoder_t));
            encoder->width = width;
            encoder->height = height;
        } else {
            LOG_INFO("Video encoder: Success!");
            return encoder;
        }
    }

    LOG_ERROR("Video encoder: All encoders failed!");
    return NULL;
}

void destroy_video_encoder(encoder_t *encoder) {
    // check if encoder encoder exists
    if (encoder == NULL) {
        LOG_WARNING("Cannot destroy encoder encoder.");
        return;
    }

    // free the ffmpeg contextes
    if (encoder->sws) {
        sws_freeContext(encoder->sws);
    }
    avcodec_free_context(&encoder->context);

    // free the encoder context and frame
    av_free(encoder->sw_frame);
    av_free(encoder->hw_frame);

    // free the buffer and encoder
    free(encoder->frame_buffer);
    free(encoder);
    return;
}

void video_encoder_set_iframe(encoder_t *encoder) {
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_I;
    encoder->sw_frame->pts +=
        encoder->context->gop_size -
        (encoder->sw_frame->pts % encoder->context->gop_size);
    encoder->sw_frame->key_frame = 1;
}

void video_encoder_unset_iframe(encoder_t *encoder) {
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_NONE;
    encoder->sw_frame->key_frame = 0;
}

void video_encoder_encode(encoder_t *encoder, void *rgb_pixels) {
    // init packet to prepare encoding
    av_packet_unref(&encoder->packet);
    av_init_packet(&encoder->packet);

    if (encoder->sws) {
        uint8_t *in_data[1];
        int in_linesize[1];

        in_data[0] = (uint8_t *)rgb_pixels;
        in_linesize[0] = encoder->width * 4;

        // convert to the encoder format
        sws_scale(encoder->sws, (const uint8_t *const *)in_data, in_linesize, 0,
                  encoder->height, encoder->sw_frame->data,
                  encoder->sw_frame->linesize);
    } else {
        memset(encoder->sw_frame->data, 0, sizeof(encoder->sw_frame->data));
        memset(encoder->sw_frame->linesize, 0,
               sizeof(encoder->sw_frame->linesize));
        encoder->sw_frame->data[0] = (uint8_t *)rgb_pixels;
        encoder->sw_frame->linesize[0] = encoder->width * 4;
    }

    int res;

    // define input data to encoder
    if (encoder->type == NVENC_ENCODE || encoder->type == QSV_ENCODE) {
        encoder->sw_frame->pts++;
        av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);

        encoder->hw_frame->pict_type = encoder->sw_frame->pict_type;

        res = avcodec_send_frame(encoder->context, encoder->hw_frame);
        if (res < 0) {
            mprintf("Could not send video hw_frame for encoding: error '%s'.\n",
                    av_err2str(res));
        }
        res = avcodec_receive_packet(encoder->context, &encoder->packet);
        if (res < 0) {
            mprintf("Could not get video packet from encoder: error '%s'.\n",
                    av_err2str(res));
        }
    } else if (encoder->type == SOFTWARE_ENCODE) {
        encoder->sw_frame->pts++;
        res = avcodec_send_frame(encoder->context, encoder->sw_frame);
        if (res < 0) {
            mprintf("Could not send video sw_frame for encoding: error '%s'.\n",
                    av_err2str(res));
        }
        res = avcodec_receive_packet(encoder->context, &encoder->packet);
        if (res < 0) {
            mprintf("Could not get video packet from encoder: error '%s'.\n",
                    av_err2str(res));
        }
    } else {
        LOG_WARNING("Invalid encoder type");
    }
}
