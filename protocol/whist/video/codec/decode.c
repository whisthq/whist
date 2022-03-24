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
#include "whist/core/error_codes.h"

#include <stdio.h>
#include <stdlib.h>

/*
============================
Private Functions
============================
*/
static enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts);
static WhistStatus try_setup_video_decoder(VideoDecoder* decoder);
static WhistStatus try_next_decoder(VideoDecoder* decoder);
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

static enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
    /*
        Determine if match_pix_fmt is in the array of pixel formats given in pix_fmts. This is
       called as a helper function in FFmpeg.

        Arguments:
            ctx (AVCodecContext*): unused, but required for the function signature by FFmpeg.
            pix_fmts (const enum AVPixelFormat*): Array of pixel formats to search through

        Returns:
            (enum AVPixelFormat): the match_pix_fmt if found; otherwise, the first software
            decode entry of pix_fmts if there is one; otherwise, AV_PIX_FMT_NONE.
    */
    VideoDecoder* decoder = ctx->opaque;

    // log all the entries of pix_fmts as supported formats.
    char supported_formats[2000] = "";
    int len = 2000;
    int i = 0;

    i += snprintf(supported_formats, len, "Supported formats:");

    for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        i += snprintf(supported_formats + i, len - i, " %s", av_get_pix_fmt_name(*p));
    }

    LOG_INFO("%s", supported_formats);

    for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == decoder->match_fmt) {
            LOG_INFO("Hardware format found, using format: %s", av_get_pix_fmt_name(*p));
            return *p;
        }
    }

    // default to the first software decode entry of pix_fmts if we couldn't find a match
    for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);
        if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) {
            continue;
        }
        LOG_WARNING("Hardware format not found, defaulting to using format: %s",
                    av_get_pix_fmt_name(*p));
        return *p;
    }

    // There were no supported formats.
    LOG_WARNING("Failed to get HW surface format.");
    return AV_PIX_FMT_NONE;
}

// This table specifies the possible hardware acceleration methods we
// can use, in the order we try them.  Software decode is tried after
// all hardware methods fail (or if it was requested explicitly).
// For example, on Windows we try D3D11VA -> DXVA2 -> NVDEC -> software
// in that order.
static const struct {
    const char* name;
    enum AVHWDeviceType type;
    enum AVPixelFormat pix_fmt;
} hardware_decode_types[] = {
#if defined(_WIN32)
    {"D3D11VA", AV_HWDEVICE_TYPE_D3D11VA, AV_PIX_FMT_D3D11},
    {"DXVA2", AV_HWDEVICE_TYPE_DXVA2, AV_PIX_FMT_DXVA2_VLD},
#endif
#if defined(__linux__)
    {"VAAPI", AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI},
#endif
#if defined(__APPLE__)
    {"VideoToolbox", AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX},
#else
    {"NVDEC", AV_HWDEVICE_TYPE_CUDA, AV_PIX_FMT_CUDA},
#endif
};
// This is used as if it were an index in the above table after all of
// the hardware entries.
static const unsigned int software_decode_type = ARRAY_LENGTH(hardware_decode_types);

static WhistStatus try_setup_video_decoder(VideoDecoder* decoder) {
    /*
        Try to setup a video decoder by checking to see if we can create AVCodecs with the specified
       device type and video codec.

        Arguments:
            decoder (VideoDecoder*): decoder struct with device type and codec type that we want to
       setup.

        Returns:
            0 on success, -1 on failure
    */
    // setup the AVCodec and AVFormatContext;

    int err;

    destroy_video_decoder_members(decoder);

    if (!decoder->can_use_hardware) {
        decoder->decode_type = software_decode_type;
    }

    const char* decoder_name;
    if (decoder->codec_type == CODEC_TYPE_H264) {
        decoder_name = "h264";
    } else if (decoder->codec_type == CODEC_TYPE_H265) {
        decoder_name = "hevc";
    } else {
        LOG_WARNING("Invalid codec type %d.", decoder->codec_type);
        return WHIST_ERROR_INVALID_ARGUMENT;
    }

    decoder->codec = avcodec_find_decoder_by_name(decoder_name);
    if (!decoder->codec) {
        LOG_WARNING("Failed to find decoder %s.", decoder_name);
        return WHIST_ERROR_EXTERNAL;
    }

    decoder->context = avcodec_alloc_context3(decoder->codec);
    if (!decoder->context) {
        LOG_WARNING("Failed to allocate decoder context.");
        return WHIST_ERROR_OUT_OF_MEMORY;
    }

    decoder->context->opaque = decoder;

    if (decoder->decode_type == software_decode_type) {
        // Software decoder.
    } else {
        enum AVHWDeviceType device_type = hardware_decode_types[decoder->decode_type].type;
        enum AVPixelFormat pix_fmt = hardware_decode_types[decoder->decode_type].pix_fmt;

        err = av_hwdevice_ctx_create(&decoder->context->hw_device_ctx, device_type, NULL, NULL, 0);
        if (err < 0) {
            LOG_WARNING("Failed to create %s hardware device for decoder: %s.",
                        av_hwdevice_get_type_name(device_type), av_err2str(err));
            return WHIST_ERROR_NOT_FOUND;
        }

        decoder->match_fmt = pix_fmt;
        decoder->context->get_format = &get_format;
    }

    if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
        LOG_WARNING("Failed to open codec for stream");
        return -1;
    }

    return 0;
}

static WhistStatus try_next_decoder(VideoDecoder* decoder) {
    // Keep trying different decode types until we find one that works.

    WhistStatus err;

    if (decoder->can_use_hardware) {
        LOG_INFO("Trying decoder type #%u", decoder->decode_type);

        while (decoder->decode_type <= software_decode_type) {
            const char* type_name;
            if (decoder->decode_type == software_decode_type)
                type_name = "software";
            else
                type_name = hardware_decode_types[decoder->decode_type].name;

            err = try_setup_video_decoder(decoder);
            if (err == WHIST_SUCCESS) {
                LOG_INFO("Video decoder: success with %s decoder!", type_name);
                if (decoder->decode_type == software_decode_type) {
                    LOG_ERROR(
                        "Video decoder: all hardware decoders failed.  "
                        "Now using software decoder.");
                }
                return err;
            }

            // That didn't work, so try the next one.
            ++decoder->decode_type;
        }

        LOG_WARNING("Video decoder: no usable decoder found!");
        return WHIST_ERROR_NOT_FOUND;
    } else {
        // if no hardware support for decoding is found, we should try software immediately.
        LOG_WARNING("Video Decoder: NO HARDWARE");
        decoder->decode_type = software_decode_type;

        err = try_setup_video_decoder(decoder);
        if (err != WHIST_SUCCESS) {
            LOG_WARNING("Video decoder: Software decoder failed!");
        } else {
            LOG_INFO("Video decoder: success with software decoder!");
        }
        return err;
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
    decoder->decode_type = 0;
    decoder->codec_type = codec_type;
    decoder->received_a_frame = false;

#ifdef __APPLE__
    decoder->can_output_hardware = true;
#else
    decoder->can_output_hardware = false;
#endif

    // Try all decoders until we find one that works
    if (try_next_decoder(decoder) != WHIST_SUCCESS) {
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

int video_decoder_send_packets(VideoDecoder* decoder, void* buffer, size_t buffer_size) {
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

    int num_packets = extract_avpackets_from_buffer(buffer, (int)buffer_size, decoder->packets);

    int res;
    for (int i = 0; i < num_packets; i++) {
        while ((res = avcodec_send_packet(decoder->context, &decoder->packets[i])) < 0) {
            LOG_WARNING("Failed to avcodec_send_packet! Error %d: %s", res, av_err2str(res));
            if (try_next_decoder(decoder) != WHIST_SUCCESS) {
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

    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(frame->format);
    if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) {
        if (decoder->can_output_hardware) {
            // The caller supports dealing with the hardware frame
            // directly, so just return it.
            decoder->decoded_frame = frame;
            decoder->using_hw = true;
        } else {
            // Otherwise, copy the hw data into a new software frame.
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
        }
    } else {
        if (decoder->decode_type != software_decode_type) {
            LOG_ERROR("Decoder internally fell back from hardware to software");
            decoder->decode_type = software_decode_type;
        }

        // We already have a software frame, so return it.
        decoder->decoded_frame = frame;
        decoder->using_hw = false;
    }

    return 0;
}

DecodedFrameData video_decoder_get_last_decoded_frame(VideoDecoder* decoder) {
    if (decoder->decoded_frame == NULL || decoder->decoded_frame->buf[0] == NULL) {
        LOG_FATAL("No decoded frame available!");
    }

    DecodedFrameData decoded_frame_data = {0};

    // Make a new reference for the caller in decoded_frame_data.  The
    // decoder's reference to the same frame is not changed.

    decoded_frame_data.decoded_frame = safe_av_frame_alloc();
    av_frame_ref(decoded_frame_data.decoded_frame, decoder->decoded_frame);

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
