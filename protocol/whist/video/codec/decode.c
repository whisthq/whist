/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file decode.c
 * @brief This file contains the code to create a video decoder and use that
 *        decoder to decode frames.
============================
Usage
============================
Video is decoded from H264 via ffmpeg; H265 is supported, but not currently used.
Hardware-accelerated decoders are given priority, but if those fail, we decode on the CPU. All
frames are eventually moved to the CPU for scaling and color conversion. Create a decoder via
create_video_decoder. To decode a frame, call video_decoder_decode on the decoder and the encoded
packets. To destroy the decoder when finished, destroy the encoder using video_decoder_decode.
*/
#if defined(_WIN32)
#pragma warning(disable : 4706)  // assignment within conditional warning
#endif

/*
============================
Includes
============================
*/
#include "decode.h"
#include <whist/logging/log_statistic.h>

#include <stdio.h>
#include <stdlib.h>

/*
============================
Private Functions
============================
*/
static void set_opt(VideoDecoder* decoder, char* option, char* value);
static void set_decoder_opts(VideoDecoder* decoder);
static int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type);
static enum AVPixelFormat match_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts,
                                       enum AVPixelFormat match_pix_fmt);
static enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts);
static int try_setup_video_decoder(VideoDecoder* decoder);
static bool try_next_decoder(VideoDecoder* decoder);
static void destroy_video_decoder_members(VideoDecoder* decoder);

#define SHOW_DECODER_LOGS false

static void swap_decoder(void* t, int t2, const char* fmt, va_list vargs) {
    UNUSED(t);
    UNUSED(t2);
    LOG_INFO("Error found");
    vprintf(fmt, vargs);
}

/*
============================
Private Function Implementations
============================
*/

static AVFrame* safe_av_frame_alloc(void) {
    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        LOG_FATAL("Failed to av_frame_alloc!");
    }
    return frame;
}

static void set_opt(VideoDecoder* decoder, char* option, char* value) {
    /*
        Wrapper function to set decoder options.

        Arguments:
            decoder (VideoDecoder*): video decoder to set options for
            option (char*): name of option as string
            value (char*): value of option as string
    */
    int ret = av_opt_set(decoder->context->priv_data, option, value, 0);
    if (ret < 0) {
        LOG_WARNING("Could not av_opt_set %s to %s for DecodeType %d!", option, value,
                    decoder->type);
    }
}

static void set_decoder_opts(VideoDecoder* decoder) {
    /*
        Set any decoder options common across all decoders.

        Arguments:
            decoder (VideoDecoder*): decoder whose options we are setting
    */
    // decoder->context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    // decoder->context->flags2 |= AV_CODEC_FLAG2_FAST;
    set_opt(decoder, "async_depth", "1");
}

static int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type) {
    /*
        Initialize a hardware-accelerated decoder with the given context and device type. Wrapper
       around FFmpeg functions to do so.

        Arguments:
            ctx (AVCodecContext*): context used to create the hardware decoder
            type (const enum AVHWDeviceType): hardware device type

        Returns:
            (int): 0 on success, negative error on failure
    */
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&ctx->hw_device_ctx, type, NULL, NULL, 0)) < 0) {
        LOG_WARNING("Failed to create specified HW device. Error %d: %s", err, av_err2str(err));
        return err;
    }

    return err;
}

static enum AVPixelFormat match_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts,
                                       enum AVPixelFormat match_pix_fmt) {
    /*
        Determine if match_pix_fmt is in the array of pixel formats given in pix_fmts. This is
       called as a helper function in FFmpeg.

        Arguments:
            ctx (AVCodecContext*): unused, but required for the function signature by FFmpeg.
            pix_fmts (const enum AVPixelFormat*): Array of pixel formats to search through
            match_pix_fmt (enum AVPixelFormat): pixel format we want to find

        Returns:
            (enum AVPixelFormat): the match_pix_fmt if found; otherwise, the first entry of pix_fmts
       if pix_fmts is a valid format; otherwise, AV_PIX_FMT_NONE.
    */
    UNUSED(ctx);

    // log all the entries of pix_fmts as supported formats.
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
            LOG_INFO("Hardware format found, using format: %s", av_get_pix_fmt_name(*p));
            return match_pix_fmt;
        }
    }

    // default to the first entry of pix_fmts if we couldn't find a match
    if (*pix_fmts != -1) {
        LOG_WARNING("Hardware format not found, defaulting to using format: %s",
                    av_get_pix_fmt_name(*pix_fmts));
        return *pix_fmts;
    }

    // There were no supported formats.
    LOG_WARNING("Failed to get HW surface format.");
    return AV_PIX_FMT_NONE;
}

static enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
    /*
        Choose a format for the video decoder. Used as a callback function in FFmpeg. This is a
       wrapper around match_fmt unless the decoder wants to use QSV format, in which case we have to
       create a different hardware device.

        Arguments:
            ctx (AVCodecContext*): the context being used for decoding
            pix_fmts (const enum AVPixelFormat*): list of format choices

        Returns:
            (enum AVPixelFormat): chosen pixel format
     */
    VideoDecoder* decoder = ctx->opaque;

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

static int try_setup_video_decoder(VideoDecoder* decoder) {
    /*
        Try to setup a video decoder by checking to see if we can create AVCodecs with the specified
       device type and video codec.

        Arguments:
            decoder (VideoDecoder*): decoder struct with device type and codec type that we want to
       setup.

        Returns:
            0 on success, -1 on failure
    */
    // setup the AVCodec and AVFormatContext

    destroy_video_decoder_members(decoder);

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
       // TODO : Support non-nvidia devices as well with AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI
        decoder->device_type = AV_HWDEVICE_TYPE_CUDA;
        decoder->match_fmt = AV_PIX_FMT_CUDA;
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

        // END HARDWARE DECODER
    } else {
        LOG_ERROR("Unsupported hardware type!");
        return -1;
    }

    return 0;
}

// Indicate in what order we should try decoding. Software is always last - we prefer hardware
// acceleration whenever possible.
#if defined(_WIN32)
static const DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_HARDWARE_OLDER,
                                                DECODE_TYPE_QSV, DECODE_TYPE_SOFTWARE};
#elif __APPLE__
static const DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_SOFTWARE};
#else  // linux
static const DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_SOFTWARE};
#endif

#define NUM_DECODER_TYPES (sizeof(decoder_precedence) / sizeof(decoder_precedence[0]))

static bool try_next_decoder(VideoDecoder* decoder) {
    /*
        Keep trying different decode types until we find one that works.

        Arguments:
            decoder (VideoDecoder*): decoder we want to set up

        Returns:
            (bool): True if we found a working decoder, False if all decoders failed
    */
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

        // loop through all decoders until we find one that works
        for (; i < NUM_DECODER_TYPES; i++) {
            decoder->type = decoder_precedence[i];
            if (try_setup_video_decoder(decoder) < 0) {
                LOG_INFO("Video decoder: Failed, trying next decoder");
            } else {
                LOG_INFO("Video decoder: Success!");
                if (decoder->type == DECODE_TYPE_SOFTWARE) {
                    LOG_ERROR(
                        "Video decoder: all hardware decoders failed. Now using software decoder.");
                }
                return true;
            }
        }

        LOG_WARNING("Video decoder: Failed, No more decoders, All decoders failed!");
        return false;
    } else {
        // if no hardware support for decoding is found, we should try software immediately.
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

static void destroy_video_decoder_members(VideoDecoder* decoder) {
    /*
        Destroy the members of the video decoder.

        Arguments:
            decoder (VideoDecoder*): decoder we are in the process of destroying
    */
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
    av_frame_free(&decoder->decoded_frame);

    // free the packets
    for (int i = 0; i < MAX_ENCODED_VIDEO_PACKETS; i++) {
        av_packet_unref(&decoder->packets[i]);
    }
    av_buffer_unref(&decoder->ref);
}

/*
============================
Public Function Implementations
============================
*/

VideoDecoder* create_video_decoder(int width, int height, bool use_hardware, CodecType codec_type) {
    /*
        Initialize a video decoder with the specified parameters.

        Arguments:
            width (int): width of the frames to decode
            height (int): height of the frames to decode
            use_hardware (bool): Whether or not we should try hardware-accelerated decoding
            codec_type (CodecType): which video codec (H264 or H265) we should use

        Returns:
            (VideoDecoder*): decoder with the specified parameters, or NULL on failure.
    */
    if (SHOW_DECODER_LOGS) {
        // av_log_set_level( AV_LOG_ERROR );
        av_log_set_callback(swap_decoder);
    }

    VideoDecoder* decoder = (VideoDecoder*)safe_malloc(sizeof(VideoDecoder));
    memset(decoder, 0, sizeof(VideoDecoder));

    decoder->width = width;
    decoder->height = height;
    decoder->can_use_hardware = use_hardware;
    decoder->type = DECODE_TYPE_NONE;
    decoder->codec_type = codec_type;
    decoder->received_a_frame = false;

    // Try all decoders until we find one that works
    if (!try_next_decoder(decoder)) {
        LOG_WARNING("No valid decoder to use");
        destroy_video_decoder(decoder);
        return NULL;
    }

    return decoder;
}

void destroy_video_decoder(VideoDecoder* decoder) {
    /*
        Destroy the video decoder and its members.

        Arguments:
            decoder (VideoDecoder*): decoder to destroy
    */
    destroy_video_decoder_members(decoder);

    // free the buffer and decoder
    free(decoder);
    return;
}

int video_decoder_send_packets(VideoDecoder* decoder, void* buffer, int buffer_size) {
    /*
        Send the packets stored in buffer to the decoder. The buffer format should be as described
       in extract_avpackets_from_buffer.

        Arguments:
            decoder (VideoDecoder*): the decoder for decoding
            buffer (void*): memory containing encoded packets
            buffer_size (int): size of buffer containing encoded packets

        Returns:
            (int): 0 on success, -1 on failure
            */

    int num_packets = extract_avpackets_from_buffer(buffer, buffer_size, decoder->packets);

    int res;
    for (int i = 0; i < num_packets; i++) {
        while ((res = avcodec_send_packet(decoder->context, &decoder->packets[i])) < 0) {
            LOG_WARNING("Failed to avcodec_send_packet! Error %d: %s", res, av_err2str(res));
            if (!try_next_decoder(decoder)) {
                for (int j = 0; j < num_packets; j++) {
                    av_packet_unref(&decoder->packets[j]);
                }
                destroy_video_decoder(decoder);
            }
            return -1;
        }
    }

    decoder->received_a_frame = true;
    return 0;
}

int video_decoder_decode_frame(VideoDecoder* decoder) {
    /*
        Get the next frame from the decoder. If we were using hardware decoding, also move the frame
       to software. At the end of this function, the decoded frame is always in decoder->sw_frame.

        Arguments:
            decoder (VideoDecoder*): the decoder we are using for decoding

        Returns:
            (int): 0 on success (can call this function again), 1 on EAGAIN (must send more input
       before calling again), -1 on failure
            */

    // get_format doesn't get called until after video_decoder_send_packets,
    // so we exit early if we haven't received a frame,
    // to ensure that get_format has been called first
    if (!decoder->received_a_frame) {
        return 1;
    }

    static WhistTimer latency_clock;

    // The frame we'll receive into
    // We can't receive into hw/sw_frame, or it'll wipe on EAGAIN.
    AVFrame* frame = safe_av_frame_alloc();
    // This might compel the software decoder to decode into NV12?
    // TODO: Is this actually necessary?
    frame->format = AV_PIX_FMT_NV12;

    // If frame was computed on the GPU
    if (decoder->context->hw_frames_ctx) {
        start_timer(&latency_clock);

        int res = avcodec_receive_frame(decoder->context, frame);
        log_double_statistic(VIDEO_AVCODEC_RECEIVE_TIME, get_timer(&latency_clock) * 1000);

        // Exit or copy the captured frame into hw_frame
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            av_frame_free(&frame);
            return 1;
        } else if (res < 0) {
            av_frame_free(&frame);
            LOG_WARNING("Failed to avcodec_receive_frame, error: %s", av_err2str(res));
            destroy_video_decoder(decoder);
            return -1;
        }

        // Free the old captured frame, if there was any
        av_frame_free(&decoder->decoded_frame);

#ifdef __APPLE__
        // On Mac, we'll use the hw frame directly
        decoder->decoded_frame = frame;
        decoder->using_hw = true;
#else
        // Otherwise, copy the hw data into a software frame
        start_timer(&latency_clock);
        decoder->decoded_frame = safe_av_frame_alloc();
        res = av_hwframe_transfer_data(decoder->decoded_frame, frame, 0);
        av_frame_free(&frame);
        if (res < 0) {
            av_frame_free(&decoder->decoded_frame);
            LOG_WARNING("Failed to av_hwframe_transfer_data, error: %s", av_err2str(res));
            destroy_video_decoder(decoder);
            return -1;
        }
        log_double_statistic(VIDEO_AV_HWFRAME_TRANSFER_TIME, get_timer(&latency_clock) * 1000);
        decoder->using_hw = false;
#endif  // #ifdef __APPLE__
    } else {
        if (decoder->type != DECODE_TYPE_SOFTWARE) {
            LOG_ERROR("Decoder cascaded from hardware to software");
            decoder->type = DECODE_TYPE_SOFTWARE;
        }

        int res = avcodec_receive_frame(decoder->context, frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            av_frame_free(&frame);
            return 1;
        } else if (res < 0) {
            av_frame_free(&frame);
            LOG_WARNING("Failed to avcodec_receive_frame, error: %s", av_err2str(res));
            destroy_video_decoder(decoder);
            return -1;
        }

        // Free the old captured frame, if there was any
        av_frame_free(&decoder->decoded_frame);
        // Move the captured frame into the decoder struct
        decoder->decoded_frame = frame;
        decoder->using_hw = false;
    }

    return 0;
}

DecodedFrameData video_decoder_get_last_decoded_frame(VideoDecoder* decoder) {
    DecodedFrameData decoded_frame_data;

    if (decoder->decoded_frame == NULL) {
        LOG_FATAL("No decoded frame available!");
    }

    // Move the frame into decoded_frame_data,
    // And then clear it from the decoder

    decoded_frame_data.decoded_frame = decoder->decoded_frame;
    decoder->decoded_frame = NULL;

    // Copy the data into the struct
    decoded_frame_data.using_hw = decoder->using_hw;
    decoded_frame_data.pixel_format = decoded_frame_data.decoded_frame->format;
    decoded_frame_data.width = decoder->width;
    decoded_frame_data.height = decoder->height;

    return decoded_frame_data;
}

void video_decoder_free_decoded_frame(DecodedFrameData* decoded_frame_data) {
    av_frame_free(&decoded_frame_data->decoded_frame);
}

#if defined(_WIN32)
#pragma warning(default : 4706)
#pragma warning(default : 4100)
#endif
