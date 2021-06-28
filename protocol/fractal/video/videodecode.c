/**
 * Copyright Fractal Computers, Inc. 2021
 * @file videodecode.c
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
#include "videodecode.h"

#include <stdio.h>
#include <stdlib.h>

/*
============================
Private Functions
============================
*/
static void set_opt(VideoDecoder* decoder, char* option, char* value);
void set_decoder_opts(VideoDecoder* decoder);
int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type);
enum AVPixelFormat match_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts,
                                enum AVPixelFormat match_pix_fmt);
enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts);
int try_setup_video_decoder(VideoDecoder* decoder);
static bool try_next_decoder(VideoDecoder* decoder);
void destroy_video_decoder_members(VideoDecoder* decoder);

#define SHOW_DECODER_LOGS false

#if SHOW_DECODER_LOGS
void swap_decoder(void* t, int t2, const char* fmt, va_list vargs);
void swap_decoder(void* t, int t2, const char* fmt, va_list vargs) {
    UNUSED(t);
    UNUSED(t2);
    LOG_INFO("Error found");
    vprintf(fmt, vargs);
}
#endif

/*
============================
Private Function Implementations
============================
*/
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
        LOG_WARNING("Could not av_opt_set %s to %s!", option, value);
    }
}

void set_decoder_opts(VideoDecoder* decoder) {
    /*
        Set any decoder options common across all decoders.

        Arguments:
            decoder (VideoDecoder*): decoder whose options we are setting
    */
    // decoder->context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    // decoder->context->flags2 |= AV_CODEC_FLAG2_FAST;
    set_opt(decoder, "async_depth", "1");
}

int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type) {
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

enum AVPixelFormat match_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts,
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
            LOG_WARNING("Hardware format found: %s", av_get_pix_fmt_name(*p));
            return *p;
        }
    }

    // default to the first entry of pix_fmts if we couldn't find a match
    if (*pix_fmts != -1) {
        LOG_WARNING("Hardware format not found, using format %s", av_get_pix_fmt_name(*pix_fmts));
        return *pix_fmts;
    }

    // There were no supported formats.
    LOG_WARNING("Failed to get HW surface format.");
    return AV_PIX_FMT_NONE;
}

enum AVPixelFormat get_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
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

int try_setup_video_decoder(VideoDecoder* decoder) {
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

// Indicate in what order we should try decoding. Software is always last - we prefer hardware
// acceleration whenever possible.
#if defined(_WIN32)
DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_HARDWARE_OLDER,
                                   DECODE_TYPE_QSV, DECODE_TYPE_SOFTWARE};
#elif __APPLE__
DecodeType decoder_precedence[] = {DECODE_TYPE_HARDWARE, DECODE_TYPE_SOFTWARE};
#else  // linux
// TODO: Fix QSV
DecodeType decoder_precedence[] = {/* DECODE_TYPE_QSV, */ DECODE_TYPE_SOFTWARE};
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

void destroy_video_decoder_members(VideoDecoder* decoder) {
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
    av_frame_free(&decoder->sw_frame);
    av_frame_free(&decoder->hw_frame);

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
#if SHOW_DECODER_LOGS
    // av_log_set_level( AV_LOG_ERROR );
    av_log_set_callback(swap_decoder);
#endif

    VideoDecoder* decoder = (VideoDecoder*)safe_malloc(sizeof(VideoDecoder));
    memset(decoder, 0, sizeof(VideoDecoder));

    decoder->width = width;
    decoder->height = height;
    decoder->can_use_hardware = use_hardware;
    decoder->type = DECODE_TYPE_NONE;
    decoder->codec_type = codec_type;

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
       in extract_packets_from_buffer.

        Arguments:
            decoder (VideoDecoder*): the decoder for decoding
            buffer (void*): memory containing encoded packets
            buffer_size (int): size of buffer containing encoded packets

        Returns:
            (int): 0 on success, -1 on failure
            */

    int num_packets = extract_packets_from_buffer(buffer, buffer_size, decoder->packets);

    int res;
    for (int i = 0; i < num_packets; i++) {
        while ((res = avcodec_send_packet(decoder->context, &decoder->packets[i])) < 0) {
            LOG_WARNING("Failed to avcodec_send_packet! Error %d: %s", res, av_err2str(res));
            if (!try_next_decoder(decoder)) {
                destroy_video_decoder(decoder);
                for (int j = 0; j < num_packets; j++) {
                    av_packet_unref(&decoder->packets[j]);
                }
            }
            return -1;
        }
    }
    return 0;
}

int video_decoder_get_frame(VideoDecoder* decoder) {
    /*
        Get the next frame from the decoder. If we were using hardware decoding, also move the frame
       to software. At the end of this function, the decoded frame is always in decoder->sw_frame.

        Arguments:
            decoder (VideoDecoder*): the decoder we are using for decoding

        Returns:
            (int): 0 on success (can call this function again), 1 on EAGAIN (must send more input
       before calling again), -1 on failure
            */
    int res;
    // If frame was computed on the GPU
    if (decoder->context->hw_frames_ctx) {
        res = avcodec_receive_frame(decoder->context, decoder->hw_frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            return 1;
        } else if (res < 0) {
            LOG_WARNING("Failed to avcodec_receive_frame, error: %s", av_err2str(res));
            destroy_video_decoder(decoder);
            return -1;
        }

        av_hwframe_transfer_data(decoder->sw_frame, decoder->hw_frame, 0);
    } else {
        if (decoder->type != DECODE_TYPE_SOFTWARE) {
            LOG_ERROR("Decoder cascaded from hardware to software");
            decoder->type = DECODE_TYPE_SOFTWARE;
        }

        res = avcodec_receive_frame(decoder->context, decoder->sw_frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            return 1;
        } else if (res < 0) {
            LOG_WARNING("Failed to avcodec_receive_frame, error: %s", av_err2str(res));
            destroy_video_decoder(decoder);
            return -1;
        }
    }
    return 0;
}

bool video_decoder_decode(VideoDecoder* decoder, void* buffer, int buffer_size) {
    /*
        Decode a frame, whose encoded data lies in buffer, using decoder, into decoder->sw_frame.

        Arguments:
            decoder (VideoDecoder*): the decoder which will decode the frame
            buffer (void*): buffer containing AVPackets and their metadata. Specifically the buffer
       contains: (n = number of packets)(s1 = size of packet 1)...(sn = size of packet n)(packet 1
       data)...(packet n data) buffer_size (int): the size of buffer, in bytes

        Returns:
            (bool): true if decode succeeded, false if failed
            */
    clock t;
    start_timer(&t);

    // copy the received packet back into the decoder AVPacket
    int* int_buffer = buffer;
    int num_packets = *int_buffer;
    int_buffer++;

    int computed_size = sizeof(int);

    // make an array of AVPacket*s and alloc each one
    AVPacket** packets = safe_malloc(num_packets * sizeof(AVPacket*));

    for (int i = 0; i < num_packets; i++) {
        // allocate packet and set size
        packets[i] = av_packet_alloc();
        packets[i]->size = *int_buffer;
        computed_size += sizeof(int) + packets[i]->size;
        int_buffer++;
    }

    if (buffer_size != computed_size) {
        LOG_ERROR(
            "Given Buffer Size did not match computed buffer size: given %d vs "
            "computed %d",
            buffer_size, computed_size);
        return false;
    }

    char* char_buffer = (void*)int_buffer;
    for (int i = 0; i < num_packets; i++) {
        // set packet data
        // we don't use av_grow_packet or av_packet_from_data:
        // the former involves allocating a new buffer and memcpying all of buffer
        // the latter needs data to be av_malloced, which is not the case for us.
        packets[i]->data = (void*)char_buffer;
        char_buffer += packets[i]->size;
    }

    for (int i = 0; i < num_packets; i++) {
        // decode the frame
        while (avcodec_send_packet(decoder->context, packets[i]) < 0) {
            LOG_WARNING("Failed to avcodec_send_packet!");
            // Try next decoder
            if (!try_next_decoder(decoder)) {
                destroy_video_decoder(decoder);
                for (int j = 0; j < num_packets; j++) {
                    av_packet_free(&packets[j]);
                }
                free(packets);
                return false;
            }
        }
    }

    for (int i = 0; i < num_packets; i++) {
        av_packet_free(&packets[i]);
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
            LOG_ERROR("Decoder cascaded from hardware to software");
            decoder->type = DECODE_TYPE_SOFTWARE;
        }

        if (avcodec_receive_frame(decoder->context, decoder->sw_frame) < 0) {
            LOG_WARNING("Failed to avcodec_receive_frame!");
            destroy_video_decoder(decoder);
            return false;
        }
    }

    double time = get_timer(t);

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
