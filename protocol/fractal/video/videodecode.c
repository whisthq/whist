/*
 * Video decoding via FFmpeg library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#if defined(_WIN32)
#pragma warning(disable : 4706)  // assignment within conditional warning
#endif

#include "videodecode.h"

#include <stdio.h>
#include <stdlib.h>

#define UNUSED(x) (void)(x)

void destroy_video_decoder_members(video_decoder_t* decoder);

#define SHOW_DECODER_LOGS false

#if SHOW_DECODER_LOGS
void swap_decoder(void* t, int t2, const char* fmt, va_list vargs) {
    t;
    t2;
    mprintf("Error found\n");
    vprintf(fmt, vargs);
}
#endif

static void set_opt(video_decoder_t* decoder, char* option, char* value) {
    int ret = av_opt_set(decoder->context->priv_data, option, value, 0);
    if (ret < 0) {
        LOG_WARNING("Could not av_opt_set %s to %s!", option, value);
    }
}

void set_decoder_opts(video_decoder_t* decoder) {
    // decoder->context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    // decoder->context->flags2 |= AV_CODEC_FLAG2_FAST;
    set_opt(decoder, "async_depth", "1");
}

int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type) {
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&ctx->hw_device_ctx, type, NULL, NULL, 0)) < 0) {
        LOG_WARNING("Failed to create specified HW device. Error %d: %s", err, av_err2str(err));
        return err;
    }

    return err;
}

enum AVPixelFormat match_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts,
                                enum AVPixelFormat match_pix_fmt) {
    UNUSED(ctx);

    char supported_formats[2000] = "";
    int len = 2000;
    int i = 0;

    i += snprintf(supported_formats, len, "Supported formats:");

    for (const enum AVPixelFormat* p = pix_fmts; *p != -1; p++) {
        i += snprintf(supported_formats + i, len - i, " %s", av_get_pix_fmt_name(*p));
    }

    LOG_INFO("%s", supported_formats);

    for (const enum AVPixelFormat* p = pix_fmts; *p != -1; p++) {
        if (*p == match_pix_fmt) {
            LOG_WARNING("Hardware format found: %s", av_get_pix_fmt_name(*p));
            return *p;
        }
    }

    if (*pix_fmts != -1) {
        LOG_WARNING("Hardware format not found, using format %s", av_get_pix_fmt_name(*pix_fmts));
        return *pix_fmts;
    }

    LOG_WARNING("Failed to get HW surface format.");
    return AV_PIX_FMT_NONE;
}

enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
    video_decoder_t* decoder = ctx->opaque;

    enum AVPixelFormat match = match_format(ctx, pix_fmts, decoder->match_fmt);

    // Create an AV_HWDEVICE_TYPE_QSV so that QSV occurs over a hardware frame
    // False, because this seems to slow down QSV for me
    if (decoder->match_fmt == AV_PIX_FMT_QSV) {
        int ret;

        ret = av_hwdevice_ctx_create(&decoder->ref, AV_HWDEVICE_TYPE_QSV, "auto", NULL, 0);
        if (ret < 0) {
            LOG_WARNING("Could not av_hwdevice_ctx_create for QSV");
            return AV_PIX_FMT_NONE;
        }

        AVHWFramesContext* frames_ctx;
        AVQSVFramesContext* frames_hwctx;

        /* create a pool of surfaces to be used by the decoder */
        ctx->hw_frames_ctx = av_hwframe_ctx_alloc(decoder->ref);
        if (!ctx->hw_frames_ctx) return AV_PIX_FMT_NONE;
        frames_ctx = (AVHWFramesContext*)ctx->hw_frames_ctx->data;
        frames_hwctx = frames_ctx->hwctx;

        frames_ctx->format = AV_PIX_FMT_QSV;
        frames_ctx->sw_format = ctx->sw_pix_fmt;
        frames_ctx->width = FFALIGN(ctx->coded_width, 32);
        frames_ctx->height = FFALIGN(ctx->coded_height, 32);
        frames_ctx->initial_pool_size = 32;

        frames_hwctx->frame_type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

        ret = av_hwframe_ctx_init(ctx->hw_frames_ctx);
        if (ret < 0) {
            LOG_WARNING("Could not av_hwframe_ctx_init for QSV");
            return AV_PIX_FMT_NONE;
        }
    }

    return match;
}

int try_setup_video_decoder(video_decoder_t* decoder) {
    // setup the AVCodec and AVFormatContext
    // avcodec_register_all is deprecated on FFmpeg 4+
    // only Linux uses FFmpeg 3.4.x because of canonical system packages
#if LIBAVCODEC_VERSION_MAJOR < 58
    LOG_INFO("OLD VERSON FFMPEG: %d.%d.%d", LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR,
             LIBAVCODEC_VERSION_MICRO);
    avcodec_register_all();
#endif

    destroy_video_decoder_members(decoder);

    int width = decoder->width;
    int height = decoder->height;

    if (decoder->type == DECODE_TYPE_SOFTWARE) {
        // BEGIN SOFTWARE DECODER
        LOG_INFO("Trying software decoder");

        if (decoder->codec_type == CODEC_TYPE_H264) {
            decoder->codec = avcodec_find_decoder_by_name("h264");
        } else if (decoder->codec_type == CODEC_TYPE_H265) {
            decoder->codec = avcodec_find_decoder_by_name("hevc");
        }

        if (!decoder->codec) {
            LOG_WARNING("Could not find video codec");
            return -1;
        }
        decoder->context = avcodec_alloc_context3(decoder->codec);
        decoder->context->opaque = decoder;

        decoder->sw_frame = (AVFrame*)av_frame_alloc();
        decoder->sw_frame->format = AV_PIX_FMT_YUV420P;
        decoder->sw_frame->width = width;
        decoder->sw_frame->height = height;
        decoder->sw_frame->pts = 0;

        if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
            LOG_WARNING("Failed to open codec for stream");
            return -1;
        }

        set_decoder_opts(decoder);
        // END SOFTWARE DECODER

    } else if (decoder->type == DECODE_TYPE_QSV) {
        // BEGIN QSV DECODER
        LOG_INFO("Trying QSV decoder");
        if (decoder->codec_type == CODEC_TYPE_H264) {
            decoder->codec = avcodec_find_decoder_by_name("h264_qsv");
        } else if (decoder->codec_type == CODEC_TYPE_H265) {
            decoder->codec = avcodec_find_decoder_by_name("hevc_qsv");
        }

        decoder->context = avcodec_alloc_context3(decoder->codec);
        decoder->context->opaque = decoder;
        set_decoder_opts(decoder);

        LOG_INFO("HWDECODER: %p", decoder->context->hw_frames_ctx);

        decoder->match_fmt = AV_PIX_FMT_QSV;
        decoder->context->get_format = get_format;

        if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
            LOG_WARNING("Failed to open context for stream");
            return -1;
        }

        decoder->sw_frame = av_frame_alloc();
        decoder->hw_frame = av_frame_alloc();

        // END QSV DECODER

    } else if (decoder->type == DECODE_TYPE_HARDWARE ||
               decoder->type == DECODE_TYPE_HARDWARE_OLDER) {
        // BEGIN HARDWARE DECODER
        LOG_INFO("Trying hardware decoder");
        // set the appropriate video decoder format based on PS
#if defined(_WIN32)
        if (decoder->type == DECODE_TYPE_HARDWARE_OLDER) {
            LOG_INFO("Will be trying older version of hardware encode");
            decoder->device_type = AV_HWDEVICE_TYPE_DXVA2;
            decoder->match_fmt = AV_PIX_FMT_DXVA2_VLD;
        } else {
            decoder->device_type = AV_HWDEVICE_TYPE_D3D11VA;
            decoder->match_fmt = AV_PIX_FMT_D3D11;
        }
#elif __APPLE__
        decoder->device_type = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
        decoder->match_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
#else  // linux
        decoder->device_type = AV_HWDEVICE_TYPE_VAAPI;
        decoder->match_fmt = AV_PIX_FMT_VAAPI;
#endif

        // get the appropriate hardware device
        if (decoder->device_type == AV_HWDEVICE_TYPE_NONE) {
            LOG_WARNING("Device type %s is not supported.",
                        av_hwdevice_get_type_name(decoder->device_type));
            LOG_WARNING("Available device types:");
            while ((decoder->device_type = av_hwdevice_iterate_types(decoder->device_type)) !=
                   AV_HWDEVICE_TYPE_NONE) {
                LOG_WARNING(" %s", av_hwdevice_get_type_name(decoder->device_type));
            }
            LOG_WARNING(" ");
            return -1;
        }

        if (decoder->codec_type == CODEC_TYPE_H264) {
            decoder->codec = avcodec_find_decoder_by_name("h264");
        } else if (decoder->codec_type == CODEC_TYPE_H265) {
            decoder->codec = avcodec_find_decoder_by_name("hevc");
        }

        if (!(decoder->context = avcodec_alloc_context3(decoder->codec))) {
            LOG_WARNING("alloccontext3 failed w/ error code: %d", AVERROR(ENOMEM));
            return -1;
        }
        decoder->context->opaque = decoder;

        decoder->context->get_format = get_format;

        if (hw_decoder_init(decoder->context, decoder->device_type) < 0) {
            LOG_WARNING("Failed to init hardware decoder");
            return -1;
        }

        if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
            LOG_WARNING("Failed to open codec for stream");
            return -1;
        }

        set_decoder_opts(decoder);

        if (!(decoder->hw_frame = av_frame_alloc()) || !(decoder->sw_frame = av_frame_alloc())) {
            LOG_WARNING("Can not alloc frame");

            av_frame_free(&decoder->hw_frame);
            av_frame_free(&decoder->sw_frame);
            return -1;
        }

        // END HARDWARE DECODER
    } else {
        LOG_ERROR("Unsupported hardware type!");
        return -1;
    }

    return 0;
}

#if defined(_WIN32)
DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_HARDWARE_OLDER,
                                   DECODE_TYPE_QSV, DECODE_TYPE_SOFTWARE};
#elif __APPLE__
DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_SOFTWARE};
#else  // linux
DecodeType decoder_precedence[] = {DECODE_TYPE_QSV, DECODE_TYPE_SOFTWARE};
#endif

#define NUM_DECODER_TYPES (sizeof(decoder_precedence) / sizeof(decoder_precedence[0]))

bool try_next_decoder(video_decoder_t* decoder) {
    if (decoder->can_use_hardware) {
        unsigned int i = 0;
        if (decoder->type != DECODE_TYPE_NONE) {
            for (; i < NUM_DECODER_TYPES; i++) {
                if (decoder->type == (DecodeType)decoder_precedence[i]) {
                    // Let's begin by trying the next one
                    i++;
                    break;
                }
            }
        }

        LOG_INFO("Trying decoder #%d", i);

        for (; i < NUM_DECODER_TYPES; i++) {
            decoder->type = decoder_precedence[i];
            if (try_setup_video_decoder(decoder) < 0) {
                LOG_INFO("Video decoder: Failed, trying next decoder");
            } else {
                LOG_INFO("Video decoder: Success!");
                return true;
            }
        }

        LOG_WARNING("Video decoder: Failed, No more decoders, All decoders failed!");
        return false;
    } else {
        LOG_WARNING("Video Decoder: NO HARDWARE");
        decoder->type = DECODE_TYPE_SOFTWARE;
        if (try_setup_video_decoder(decoder) < 0) {
            LOG_WARNING("Video decoder: Software decoder failed!");
            return false;
        } else {
            LOG_INFO("Video decoder: Success!");
            return true;
        }
    }
}

video_decoder_t* create_video_decoder(int width, int height, bool use_hardware,
                                      CodecType codec_type) {
#if SHOW_DECODER_LOGS
    // av_log_set_level( AV_LOG_ERROR );
    av_log_set_callback(swap_decoder);
#endif

    video_decoder_t* decoder = (video_decoder_t*)malloc(sizeof(video_decoder_t));
    memset(decoder, 0, sizeof(video_decoder_t));

    decoder->width = width;
    decoder->height = height;
    decoder->can_use_hardware = use_hardware;
    decoder->type = DECODE_TYPE_NONE;
    decoder->codec_type = codec_type;

    if (!try_next_decoder(decoder)) {
        destroy_video_decoder(decoder);
        return NULL;
    }

    return decoder;
}

/// @brief destroy decoder decoder
/// @details frees FFmpeg decoder memory

void destroy_video_decoder(video_decoder_t* decoder) {
    destroy_video_decoder_members(decoder);

    // free the buffer and decoder
    free(decoder);
    return;
}

void destroy_video_decoder_members(video_decoder_t* decoder) {
    // check if decoder decoder exists
    if (decoder == NULL) {
        LOG_WARNING("Cannot destroy decoder decoder.");
        return;
    }

    /* flush the decoder */
    avcodec_free_context(&decoder->context);

    // free the ffmpeg contextes
    avcodec_close(decoder->context);

    // free the decoder context and frame
    av_free(decoder->context);
    av_frame_free(&decoder->sw_frame);
    av_frame_free(&decoder->hw_frame);
    av_buffer_unref(&decoder->ref);
}

/// @brief decode a frame using the decoder decoder
/// @details decode an encoded frame under YUV color format into RGB frame
bool video_decoder_decode(video_decoder_t* decoder, void* buffer, int buffer_size) {
    clock t;
    StartTimer(&t);

    // copy the received packet back into the decoder AVPacket
    // memcpy(&decoder->packet.data, &buffer, buffer_size);
    int* int_buffer = buffer;
    int num_packets = *int_buffer;
    int_buffer++;

    int computed_size = 4;

    AVPacket* packets = malloc(num_packets * sizeof(AVPacket));

    for (int i = 0; i < num_packets; i++) {
        av_init_packet(&packets[i]);
        packets[i].size = *int_buffer;
        computed_size += 4 + packets[i].size;
        int_buffer++;
    }

    if (buffer_size != computed_size) {
        LOG_ERROR(
            "Given Buffer Size did not match computed buffer size: given %d vs "
            "computed %d",
            buffer_size, computed_size);
    }

    char* char_buffer = (void*)int_buffer;
    for (int i = 0; i < num_packets; i++) {
        packets[i].data = (void*)char_buffer;
        char_buffer += packets[i].size;
    }

    for (int i = 0; i < num_packets; i++) {
        // decode the frame
        while (avcodec_send_packet(decoder->context, &packets[i]) < 0) {
            LOG_WARNING("Failed to avcodec_send_packet!");
            if (!try_next_decoder(decoder)) {
                destroy_video_decoder(decoder);
                for (int j = 0; j < num_packets; j++) {
                    av_packet_unref(&packets[j]);
                }
                free(packets);
                return false;
            }
        }
    }

    for (int i = 0; i < num_packets; i++) {
        av_packet_unref(&packets[i]);
    }
    free(packets);

    // If frame was computed on the CPU
    if (decoder->context->hw_frames_ctx) {
        // If frame was computed on the GPU
        if (avcodec_receive_frame(decoder->context, decoder->hw_frame) < 0) {
            LOG_WARNING("Failed to avcodec_receive_frame!");
            destroy_video_decoder(decoder);
            return false;
        }

        av_hwframe_transfer_data(decoder->sw_frame, decoder->hw_frame, 0);
    } else {
        if (decoder->type != DECODE_TYPE_SOFTWARE) {
            LOG_INFO("Decoder cascaded from hardware to software");
            decoder->type = DECODE_TYPE_SOFTWARE;
        }

        if (avcodec_receive_frame(decoder->context, decoder->sw_frame) < 0) {
            LOG_WARNING("Failed to avcodec_receive_frame!");
            destroy_video_decoder(decoder);
            return false;
        }
    }

    double time = GetTimer(t);

    static double total_time = 0.0;
    static double max_time = 0.0;
    static int num_times = 0;

    total_time += time;
    max_time = max(max_time, time);
    num_times++;

    if (num_times == 10) {
        LOG_INFO("Avg Decode Time: %f\n", total_time / num_times);
        total_time = 0.0;
        max_time = 0.0;
        num_times = 0;
    }
    return true;
}

#if defined(_WIN32)
#pragma warning(default : 4706)
#pragma warning(default : 4100)
#endif
