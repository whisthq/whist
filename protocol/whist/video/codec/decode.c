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
#include "whist/utils/command_line.h"
#include "whist/utils/string_buffer.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <libavutil/hwcontext_d3d11va.h>
#endif

static const char* save_decoder_input;
COMMAND_LINE_STRING_OPTION(save_decoder_input, 0, "save-decoder-input", 256,
                           "Save decoder input to a file.")

/*
============================
Private Functions
============================
*/
static WhistStatus try_setup_video_decoder(VideoDecoder* decoder);
static WhistStatus try_next_decoder(VideoDecoder* decoder);
static void destroy_video_decoder_members(VideoDecoder* decoder);

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

static bool find_pix_fmt(const enum AVPixelFormat* list, enum AVPixelFormat format) {
    for (int i = 0; list[i] != AV_PIX_FMT_NONE; i++) {
        if (list[i] == format) {
            return true;
        }
    }
    return false;
}

static void log_pix_fmts(const enum AVPixelFormat* pix_fmts) {
    // log all the entries of pix_fmts as supported formats.
    STRING_BUFFER_LOCAL(supported_formats, 256);
    for (const enum AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        string_buffer_printf(&supported_formats, " %s", av_get_pix_fmt_name(*p));
    }
    LOG_INFO("Supported formats:%s.", string_buffer_string(&supported_formats));
}

static enum AVPixelFormat get_format_software(AVCodecContext* avctx,
                                              const enum AVPixelFormat* pix_fmts) {
    // Use the first software format in the list.
    for (int i = 0; pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(pix_fmts[i]);
        if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) {
            continue;
        }
        LOG_INFO("Using software format %s.", av_get_pix_fmt_name(pix_fmts[i]));
        return pix_fmts[i];
    }

    // There were no supported formats.
    LOG_WARNING("No usable decoding format found.");
    return AV_PIX_FMT_NONE;
}

#ifdef _WIN32
static enum AVPixelFormat get_format_d3d11(AVCodecContext* avctx,
                                           const enum AVPixelFormat* pix_fmts) {
    AVBufferRef* frames_ref = NULL;
    int err;

    log_pix_fmts(pix_fmts);

    if (!find_pix_fmt(pix_fmts, AV_PIX_FMT_D3D11)) {
        LOG_WARNING("Expected D3D11 format not found, falling back to software.");
        goto fail;
    }

    err = avcodec_get_hw_frames_parameters(avctx, avctx->hw_device_ctx, AV_PIX_FMT_D3D11,
                                           &frames_ref);
    if (err < 0) {
        LOG_WARNING("Failed to get D3D11 hardware frame parameters: %s.", av_err2str(err));
        goto fail;
    }

    AVHWFramesContext* frames = (AVHWFramesContext*)frames_ref->data;
    AVD3D11VAFramesContext* d3d11 = frames->hwctx;

    // We normally want to use the returned frames as a shader resource
    // (e.g. for rendering within SDL).  This flag is not needed if they
    // are instead copied to CPU memory, but it is easier to always set
    // it.
    d3d11->BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    // If run in a multithreaded context then we will want to share the
    // frames with the render device.
    d3d11->MiscFlags |= D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    err = av_hwframe_ctx_init(frames_ref);
    if (err < 0) {
        LOG_WARNING("Failed to init D3D11 hardware frame context: %s.", av_err2str(err));
        goto fail;
    }

    avctx->hw_frames_ctx = frames_ref;
    LOG_INFO("Using D3D11 hardware format.");
    return AV_PIX_FMT_D3D11;

fail:
    av_buffer_unref(&frames_ref);
    return get_format_software(avctx, pix_fmts);
}
#endif

static enum AVPixelFormat get_format_default(AVCodecContext* avctx,
                                             const enum AVPixelFormat* pix_fmts) {
    VideoDecoder* decoder = avctx->opaque;

    log_pix_fmts(pix_fmts);

    if (find_pix_fmt(pix_fmts, decoder->match_fmt)) {
        LOG_INFO("Using matched hardware format %s.", av_get_pix_fmt_name(decoder->match_fmt));
        return decoder->match_fmt;
    }

    // If we didn't find a match, fall back to software decoding.
    return get_format_software(avctx, pix_fmts);
}

typedef struct {
    // Nice name for log messages.
    const char* name;
    // Hardware device type needed for this mode.
    enum AVHWDeviceType device_type;
    // Pixel format which we will want to return in this mode.
    enum AVPixelFormat pix_fmt;
    // Custom get_format() callback for additional setup.  Can be NULL,
    // in which case a default which just matches the pix_fmt is used.
    enum AVPixelFormat (*get_format)(AVCodecContext* avctx, const enum AVPixelFormat* pix_fmts);
} HardwareDecodeType;

// This table specifies the possible hardware acceleration methods we
// can use, in the order we try them.  Software decode is tried after
// all hardware methods fail (or if it was requested explicitly).
// For example, on Windows we try D3D11VA -> DXVA2 -> NVDEC -> software
// in that order.
static const HardwareDecodeType hardware_decode_types[] = {
#if defined(_WIN32)
    {"D3D11VA", AV_HWDEVICE_TYPE_D3D11VA, AV_PIX_FMT_D3D11, &get_format_d3d11},
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

    if (!decoder->params.hardware_decode) {
        decoder->decode_type = software_decode_type;
    }

    const char* decoder_name;
    if (decoder->params.codec_type == CODEC_TYPE_H264) {
        decoder_name = "h264";
    } else if (decoder->params.codec_type == CODEC_TYPE_H265) {
        decoder_name = "hevc";
    } else {
        LOG_WARNING("Invalid codec type %d.", decoder->params.codec_type);
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
        decoder->context->get_format = &get_format_software;
    } else {
        const HardwareDecodeType* hw = &hardware_decode_types[decoder->decode_type];

        bool create_new_device = true;
        if (decoder->params.hardware_device) {
            // User-supplied hardware device is present - use it if it
            // matches the decode type we are using.
            AVHWDeviceContext* dev = (AVHWDeviceContext*)decoder->params.hardware_device->data;
            if (dev->type == hw->device_type) {
                decoder->context->hw_device_ctx = av_buffer_ref(decoder->params.hardware_device);
                if (decoder->context->hw_device_ctx == NULL) {
                    LOG_WARNING("Failed to reference user-supplied device.");
                    return WHIST_ERROR_EXTERNAL;
                }
                LOG_INFO("Using existing %s device for decoder.",
                         av_hwdevice_get_type_name(dev->type));
                create_new_device = false;
            } else {
                // If it doesn't match then we will create a new device.
                // This happens if, for example, we supply a D3D11
                // device (because we would like to share a device with
                // the output to avoid copies), but that doesn't work
                // so we fall back to DXVA2 (and will then need to copy,
                // but can still hardware decode).
            }
        }

        if (create_new_device) {
            // Make a new device here.
            err = av_hwdevice_ctx_create(&decoder->context->hw_device_ctx, hw->device_type, NULL,
                                         NULL, 0);
            if (err < 0) {
                LOG_WARNING("Failed to create %s hardware device for decoder: %s.",
                            av_hwdevice_get_type_name(hw->device_type), av_err2str(err));
                return WHIST_ERROR_NOT_FOUND;
            }
            LOG_INFO("Created new %s device for decoder.",
                     av_hwdevice_get_type_name(hw->device_type));
        }

        if (hw->get_format) {
            decoder->context->get_format = hw->get_format;
        } else {
            decoder->match_fmt = hw->pix_fmt;
            decoder->context->get_format = &get_format_default;
        }
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

    if (decoder->params.hardware_decode) {
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
        av_packet_free(&decoder->packets[i]);
    }
    av_buffer_unref(&decoder->ref);
}

/*
============================
Public Function Implementations
============================
*/

VideoDecoder* video_decoder_create(const VideoDecoderParams* params) {
    VideoDecoder* decoder = (VideoDecoder*)safe_malloc(sizeof(VideoDecoder));
    memset(decoder, 0, sizeof(VideoDecoder));

    decoder->params = *params;

    decoder->decode_type = 0;
    decoder->received_a_frame = false;

    // Try all decoders until we find one that works
    if (try_next_decoder(decoder) != WHIST_SUCCESS) {
        LOG_WARNING("No valid decoder to use");
        destroy_video_decoder(decoder);
        return NULL;
    }

    if (save_decoder_input) {
        decoder->save_input_file = fopen(save_decoder_input, "wb");
        if (decoder->save_input_file == NULL) {
            LOG_FATAL("Failed to open \"%s\" to save decoder input.", save_decoder_input);
        }
    }

    return decoder;
}

VideoDecoder* create_video_decoder(int width, int height, bool use_hardware, CodecType codec_type) {
    VideoDecoderParams params = {
        .codec_type = codec_type,
        .width = width,
        .height = height,
        .hardware_decode = use_hardware,
        .hardware_output_format = AV_PIX_FMT_NONE,

    };
    return video_decoder_create(&params);
}

void destroy_video_decoder(VideoDecoder* decoder) {
    /*
        Destroy the video decoder and its members.

        Arguments:
            decoder (VideoDecoder*): decoder to destroy
    */
    destroy_video_decoder_members(decoder);

    if (save_decoder_input) {
        fclose(decoder->save_input_file);
    }

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

    int num_packets = extract_avpackets_from_buffer(buffer, buffer_size, decoder->packets);

    if (save_decoder_input) {
        for (int i = 0; i < num_packets; i++) {
            AVPacket* pkt = decoder->packets[i];
            fwrite(pkt->data, pkt->size, 1, decoder->save_input_file);
        }
        // Flush after every write - if the decoder fails on this input
        // then we won't have an opportunity to flush later.
        fflush(decoder->save_input_file);
    }

    int res;
    for (int i = 0; i < num_packets; i++) {
        AVPacket* pkt = decoder->packets[i];

        res = avcodec_send_packet(decoder->context, pkt);
        if (res < 0) {
            LOG_WARNING("Failed to avcodec_send_packet! Error %d: %s", res, av_err2str(res));
            if (try_next_decoder(decoder) != WHIST_SUCCESS) {
                destroy_video_decoder(decoder);
                return -1;
            } else {
                // Tail call this function again.
                return video_decoder_send_packets(decoder, buffer, buffer_size);
            }
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
        if (decoder->params.hardware_output_format == frame->format) {
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
    decoded_frame_data.width = decoder->params.width;
    decoded_frame_data.height = decoder->params.height;

    return decoded_frame_data;
}

void video_decoder_free_decoded_frame(DecodedFrameData* decoded_frame_data) {
    av_frame_free(&decoded_frame_data->decoded_frame);
}

#if defined(_WIN32)
#pragma warning(default : 4706)
#pragma warning(default : 4100)
#endif
