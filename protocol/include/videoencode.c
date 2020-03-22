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
        enum AVPixelFormat out_format = AV_PIX_FMT_0RGB32;

        mprintf("Trying Nvidia encoder\n");
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
        encoder->context->width = encoder->out_width;
        encoder->context->height = encoder->out_height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = 2 * bitrate;
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

        if (avcodec_open2(encoder->context, encoder->codec, NULL, NULL) < 0) {
            mprintf("Failed to open video encoder context\n");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->out_width;
        encoder->sw_frame->height = encoder->out_height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = avpicture_get_size(out_format, encoder->out_width,
                                            encoder->out_height);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        avpicture_fill((AVPicture *)encoder->sw_frame,
                       (uint8_t *)encoder->frame_buffer, out_format,
                       encoder->out_width, encoder->out_height);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(encoder->in_width, encoder->in_height,
                                      AV_PIX_FMT_RGB32, encoder->out_width,
                                      encoder->out_height, out_format,
                                      SWS_BICUBIC, 0, 0, 0);
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
        encoder->context->width = encoder->out_width;
        encoder->context->height = encoder->out_height;
        encoder->context->bit_rate = bitrate;
        encoder->context->rc_max_rate = 2 * bitrate;
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

        if (avcodec_open2(encoder->context, encoder->codec, NULL, NULL) < 0) {
            mprintf("Failed to open video encoder context\n");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = out_format;
        encoder->sw_frame->width = encoder->out_width;
        encoder->sw_frame->height = encoder->out_height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = avpicture_get_size(out_format, encoder->out_width,
                                            encoder->out_height);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        avpicture_fill((AVPicture *)encoder->sw_frame,
                       (uint8_t *)encoder->frame_buffer, out_format,
                       encoder->out_width, encoder->out_height);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(encoder->in_width, encoder->in_height,
                                      AV_PIX_FMT_RGB32, encoder->out_width,
                                      encoder->out_height, out_format,
                                      SWS_BICUBIC, 0, 0, 0);
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

        encoder->codec = avcodec_find_encoder(AV_CODEC_ID_H264);

        encoder->context = avcodec_alloc_context3(encoder->codec);
        encoder->context->width = encoder->out_width;
        encoder->context->height = encoder->out_height;
        encoder->context->bit_rate = bitrate;
        encoder->context->time_base.num = 1;
        encoder->context->time_base.den = 30;
        encoder->context->gop_size = gop_size;
        encoder->context->pix_fmt = AV_PIX_FMT_YUV420P;

        av_opt_set(encoder->context->priv_data, "preset", "ultrafast", 0);
        av_opt_set(encoder->context->priv_data, "tune", "zerolatency", 0);

        if (avcodec_open2(encoder->context, encoder->codec, NULL, NULL) < 0) {
            mprintf("Failed to open context for stream\n");
            return -1;
        }

        encoder->sw_frame = (AVFrame *)av_frame_alloc();
        encoder->sw_frame->format = AV_PIX_FMT_YUV420P;
        encoder->sw_frame->width = encoder->out_width;
        encoder->sw_frame->height = encoder->out_height;
        encoder->sw_frame->pts = 0;

        // set frame size and allocate memory for it
        int frame_size = avpicture_get_size(
            AV_PIX_FMT_YUV420P, encoder->out_width, encoder->out_height);
        encoder->frame_buffer = malloc(frame_size);

        // fill picture with empty frame buffer
        avpicture_fill((AVPicture *)encoder->sw_frame,
                       (uint8_t *)encoder->frame_buffer, AV_PIX_FMT_YUV420P,
                       encoder->out_width, encoder->out_height);

        // set sws context for color format conversion
        encoder->sws = sws_getContext(encoder->in_width, encoder->in_height,
                                      AV_PIX_FMT_RGB32, encoder->out_width,
                                      encoder->out_height, AV_PIX_FMT_YUV420P,
                                      SWS_BICUBIC, 0, 0, 0);
        if (!encoder->sws) {
            mprintf("Failed to initialize swsContext for video encoder\n");
            return -1;
        }
    } else {
        mprintf("Unsupported hardware type!\n");
        return -1;
    }

    return 0;
}

/// @brief creates encoder encoder
/// @details creates FFmpeg encoder
encoder_t *create_video_encoder(int in_width, int in_height, int out_width,
                                int out_height, int bitrate, int gop_size) {
    // set memory for the encoder
    encoder_t *encoder = (encoder_t *)malloc(sizeof(encoder_t));
    memset(encoder, 0, sizeof(encoder_t));

    // get frame dimensions for encoder
    encoder->in_width = in_width;
    encoder->in_height = in_height;
    encoder->out_width = out_width;
    encoder->out_height = out_height;

    int encoder_precedence[] = {NVENC_ENCODE, QSV_ENCODE, SOFTWARE_ENCODE};

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
void video_encoder_encode(encoder_t *encoder, void *rgb_pixels) {
    // define input data to encoder
    memset(encoder->sw_frame->data, 0, sizeof(encoder->sw_frame->data));
    memset(encoder->sw_frame->linesize, 0, sizeof(encoder->sw_frame->linesize));
    encoder->sw_frame->data[0] = (uint8_t *)rgb_pixels;
    encoder->sw_frame->linesize[0] = encoder->in_width * 4;

    // init packet to prepare encoding
    av_packet_unref(&encoder->packet);

    av_init_packet(&encoder->packet);
    int success = 0;  // boolean for success or failure of encoding

    // attempt to encode the frame
    if (encoder->type == NVENC_ENCODE) {
        av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);

        avcodec_encode_video2(encoder->context, &encoder->packet,
                              encoder->hw_frame, &success);
    } else {
        avcodec_encode_video2(encoder->context, &encoder->packet,
                              encoder->sw_frame, &success);
    }
}
