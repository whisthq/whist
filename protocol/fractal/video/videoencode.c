/**
 * Copyright Fractal Computers, Inc. 2020
 * @file videoencode.c
 * @brief This file contains the code to create and destroy Encoders and use
 *        them to encode captured screens.
============================
Usage
============================
Video is encoded to H264 via either a hardware encoder (currently, we use NVidia GPUs, so we use
NVENC) or a software encoder. H265 is also supported but not currently used. Since NVidia allows us
to both capture and encode the screen, most of the functions will be called in server/main.c with an
empty dummy encoder. For encoders, create an H264 encoder via create_video_encode, and use
it to encode frames via video_encoder_encode. Write the encoded output via
video_encoder_write_buffer, and when finished, destroy the encoder using destroy_video_encoder.
*/

/*
============================
Includes
============================
*/
#include "videoencode.h"

#include <libavfilter/avfilter.h>
#include <stdio.h>
#include <stdlib.h>

#define GOP_SIZE 99999
#define MIN_NVENC_WIDTH 33
#define MIN_NVENC_HEIGHT 17
/*
============================
Private Functions
============================
*/
int video_encoder_receive_packet(VideoEncoder *encoder, AVPacket *packet);
int video_encoder_send_frame(VideoEncoder *encoder);
void set_opt(VideoEncoder *encoder, char *option, char *value);
VideoEncoder *create_nvenc_encoder(int in_width, int in_height, int out_width, int out_height,
                                   int bitrate, CodecType codec_type);
VideoEncoder *create_qsv_encoder(int in_width, int in_height, int out_width, int out_height,
                                 int bitrate, CodecType codec_type);
VideoEncoder *create_sw_encoder(int in_width, int in_height, int out_width, int out_height,
                                int bitrate, CodecType codec_type);

/*
============================
Private Function Implementations
============================
*/
void set_opt(VideoEncoder *encoder, char *option, char *value) {
    /*
        Wrapper function to set encoder options, like presets, latency, and bitrate.

        Arguments:
            encoder (VideoEncoder*): video encoder to set options for
            option (char*): name of option as string
            value (char*): value of option as string
    */
    int ret = av_opt_set(encoder->context->priv_data, option, value, 0);
    if (ret < 0) {
        LOG_WARNING("Could not av_opt_set %s to %s!", option, value);
    }
}

typedef VideoEncoder *(*VideoEncoderCreator)(int, int, int, int, int, CodecType);

VideoEncoder *create_nvenc_encoder(int in_width, int in_height, int out_width, int out_height,
                                   int bitrate, CodecType codec_type) {
    /*
        Create an encoder using Nvidia's video encoding alorithms.

        Arguments:
            in_width (int): Width of the frames that the encoder intakes
            in_height (int): height of the frames that the encoder intakes
            out_width (int): width of the frames that the encoder outputs
            out_height (int): Height of the frames that the encoder outputs
            bitrate (int): bits per second the encoder will encode to
            codec_type (CodecType): Codec (currently H264 or H265) the encoder will use

        Returns:
            (VideoEncoder*): the newly created encoder
     */
    LOG_INFO("Trying NVENC encoder...");
    VideoEncoder *encoder = (VideoEncoder *)safe_malloc(sizeof(VideoEncoder));
    memset(encoder, 0, sizeof(VideoEncoder));

    encoder->type = NVENC_ENCODE;
    encoder->in_width = in_width;
    encoder->in_height = in_height;
    if (out_width <= 32) out_width = MIN_NVENC_WIDTH;
    if (out_height <= 16) out_height = MIN_NVENC_HEIGHT;
    encoder->out_width = out_width;
    encoder->out_height = out_height;
    encoder->codec_type = codec_type;
    encoder->gop_size = GOP_SIZE;
    encoder->frames_since_last_iframe = 0;

    enum AVPixelFormat in_format = AV_PIX_FMT_RGB32;
    enum AVPixelFormat hw_format = AV_PIX_FMT_CUDA;
    enum AVPixelFormat sw_format = AV_PIX_FMT_0RGB32;

    // init intake format in sw_frame

    encoder->sw_frame = av_frame_alloc();
    encoder->sw_frame->format = in_format;
    encoder->sw_frame->width = encoder->in_width;
    encoder->sw_frame->height = encoder->in_height;
    encoder->sw_frame->pts = 0;

    // set frame size and allocate memory for it
    int frame_size =
        av_image_get_buffer_size(in_format, encoder->out_width, encoder->out_height, 1);
    encoder->sw_frame_buffer = safe_malloc(frame_size);

    // fill picture with empty frame buffer
    av_image_fill_arrays(encoder->sw_frame->data, encoder->sw_frame->linesize,
                         (uint8_t *)encoder->sw_frame_buffer, in_format, encoder->out_width,
                         encoder->out_height, 1);

    // init hw_device_ctx
    if (av_hwdevice_ctx_create(&encoder->hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, "CUDA", NULL, 0) <
        0) {
        LOG_WARNING("Failed to create hardware device context");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init encoder format in context

    if (encoder->codec_type == CODEC_TYPE_H264) {
        encoder->codec = avcodec_find_encoder_by_name("h264_nvenc");
    } else if (encoder->codec_type == CODEC_TYPE_H265) {
        encoder->codec = avcodec_find_encoder_by_name("hevc_nvenc");
    }

    encoder->context = avcodec_alloc_context3(encoder->codec);
    encoder->context->width = encoder->out_width;
    encoder->context->height = encoder->out_height;
    encoder->context->bit_rate = bitrate;
    encoder->context->rc_max_rate = bitrate;
    encoder->context->rc_buffer_size = 4 * (bitrate / FPS);
    encoder->context->time_base.num = 1;
    encoder->context->time_base.den = FPS;
    encoder->context->gop_size = encoder->gop_size;
    encoder->context->keyint_min = 5;
    encoder->context->pix_fmt = hw_format;

    // enable automatic insertion of non-reference P-frames
    set_opt(encoder, "nonref_p", "1");
    // llhq is deprecated - we are now supposed to use p1-p7 and tune
    // p1: fastest, but lowest quality -- p7: slowest, best quality
    // only constqp/cbr/vbr are supported now with these presets
    // tune: high quality, low latency, ultra low latency, or lossless; we use ultra low latency
    set_opt(encoder, "preset", "p1");
    set_opt(encoder, "tune", "ull");
    set_opt(encoder, "rc", "cbr");
    // zerolatency: no reordering delay
    set_opt(encoder, "zerolatency", "1");
    // delay frame output by 0 frames
    set_opt(encoder, "delay", "0");

    // assign hw_device_ctx
    av_buffer_unref(&encoder->context->hw_frames_ctx);
    encoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(encoder->hw_device_ctx);

    // init HWFramesContext
    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)encoder->context->hw_frames_ctx->data;
    frames_ctx->format = hw_format;
    frames_ctx->sw_format = sw_format;
    frames_ctx->width = encoder->in_width;
    frames_ctx->height = encoder->in_height;
    if (av_hwframe_ctx_init(encoder->context->hw_frames_ctx) < 0) {
        LOG_WARNING("Failed to initialize hardware frames context");
        destroy_video_encoder(encoder);
        return NULL;
    }

    if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
        LOG_WARNING("Failed to open context for stream");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init hardware frame
    encoder->hw_frame = av_frame_alloc();
    int res = av_hwframe_get_buffer(encoder->context->hw_frames_ctx, encoder->hw_frame, 0);
    if (res < 0) {
        LOG_WARNING("Failed to init buffer for video encoder hw frames: %s", av_err2str(res));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init resizing in filter_graph

    encoder->filter_graph = avfilter_graph_alloc();
    if (!encoder->filter_graph) {
        LOG_WARNING("Unable to create filter graph");
        destroy_video_encoder(encoder);
        return NULL;
    }

#if defined(_WIN32)
// windows currently supports hw resize
#define N_FILTERS_NVENC 3
    // source -> scale_cuda -> sink
    const AVFilter *filters[N_FILTERS_NVENC] = {0};
    filters[0] = avfilter_get_by_name("buffer");
    filters[1] = avfilter_get_by_name("scale_cuda");
    filters[2] = avfilter_get_by_name("buffersink");

    for (int i = 0; i < N_FILTERS_NVENC; ++i) {
        if (!filters[i]) {
            LOG_WARNING("Could not find filter %d in the list!", i);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    AVFilterContext *filter_contexts[N_FILTERS_NVENC] = {0};

    // source buffer
    filter_contexts[0] = avfilter_graph_alloc_filter(encoder->filter_graph, filters[0], "src");
    AVBufferSrcParameters *avbsp = av_buffersrc_parameters_alloc();
    avbsp->width = encoder->in_width;
    avbsp->height = encoder->in_height;
    avbsp->format = hw_format;
    avbsp->frame_rate = (AVRational){FPS, 1};
    avbsp->time_base = (AVRational){1, FPS};
    avbsp->hw_frames_ctx = encoder->context->hw_frames_ctx;
    av_buffersrc_parameters_set(filter_contexts[0], avbsp);
    if (avfilter_init_str(filter_contexts[0], NULL) < 0) {
        LOG_WARNING("Unable to initialize buffer source");
        destroy_video_encoder(encoder);
        return NULL;
    }
    av_free(avbsp);
    encoder->filter_graph_source = filter_contexts[0];

    // scale_cuda
    filter_contexts[1] =
        avfilter_graph_alloc_filter(encoder->filter_graph, filters[1], "scale_cuda");
    char options_string[60] = "";
    snprintf(options_string, 60, "w=%d:h=%d", encoder->out_width, encoder->out_height);
    if (avfilter_init_str(filter_contexts[1], options_string) < 0) {
        LOG_WARNING("Unable to initialize scale filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // sink buffer
    if (avfilter_graph_create_filter(&filter_contexts[2], filters[2], "sink", NULL, NULL,
                                     encoder->filter_graph) < 0) {
        LOG_WARNING("Unable to initialize buffer sink");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->filter_graph_sink = filter_contexts[2];
#else
// linux currently doesn't support hw resize
#define N_FILTERS_NVENC 2
    // source -> sink
    const AVFilter *filters[N_FILTERS_NVENC] = {0};
    filters[0] = avfilter_get_by_name("buffer");
    filters[1] = avfilter_get_by_name("buffersink");

    for (int i = 0; i < N_FILTERS_NVENC; ++i) {
        if (!filters[i]) {
            LOG_WARNING("Could not find filter %d in the list!", i);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    AVFilterContext *filter_contexts[N_FILTERS_NVENC] = {0};

    // source buffer
    filter_contexts[0] = avfilter_graph_alloc_filter(encoder->filter_graph, filters[0], "src");
    AVBufferSrcParameters *avbsp = av_buffersrc_parameters_alloc();
    avbsp->width = encoder->in_width;
    avbsp->height = encoder->in_height;
    avbsp->format = hw_format;
    avbsp->frame_rate = (AVRational){FPS, 1};
    avbsp->time_base = (AVRational){1, FPS};
    avbsp->hw_frames_ctx = encoder->context->hw_frames_ctx;
    av_buffersrc_parameters_set(filter_contexts[0], avbsp);
    if (avfilter_init_str(filter_contexts[0], NULL) < 0) {
        LOG_WARNING("Unable to initialize buffer source");
        destroy_video_encoder(encoder);
        return NULL;
    }
    av_free(avbsp);
    encoder->filter_graph_source = filter_contexts[0];
    // sink buffer
    if (avfilter_graph_create_filter(&filter_contexts[1], filters[1], "sink", NULL, NULL,
                                     encoder->filter_graph) < 0) {
        LOG_WARNING("Unable to initialize buffer sink");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->filter_graph_sink = filter_contexts[1];
#endif

    // connect the filters in a simple line
    for (int i = 0; i < N_FILTERS_NVENC - 1; ++i) {
        if (avfilter_link(filter_contexts[i], 0, filter_contexts[i + 1], 0) < 0) {
            LOG_WARNING("Unable to link filters %d to %d", i, i + 1);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    int err = avfilter_graph_config(encoder->filter_graph, NULL);
    if (err < 0) {
        LOG_WARNING("Unable to configure the filter graph: %s", av_err2str(err));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init transfer frame
    encoder->filtered_frame = av_frame_alloc();

    return encoder;
}

VideoEncoder *create_qsv_encoder(int in_width, int in_height, int out_width, int out_height,
                                 int bitrate, CodecType codec_type) {
    /*
        Create a QSV (Intel Quick Sync Video) encoder for Intel hardware encoding.

        Arguments:
            in_width (int): Width of the frames that the encoder intakes
            in_height (int): height of the frames that the encoder intakes
            out_width (int): width of the frames that the encoder outputs
            out_height (int): Height of the frames that the encoder outputs
            bitrate (int): bits per second the encoder will encode to
            codec_type (CodecType): Codec (currently H264 or H265) the encoder will use

        Returns:
            (VideoEncoder*): the newly created encoder
     */
    LOG_INFO("Trying QSV encoder...");
    VideoEncoder *encoder = (VideoEncoder *)safe_malloc(sizeof(VideoEncoder));
    memset(encoder, 0, sizeof(VideoEncoder));

    encoder->type = QSV_ENCODE;
    encoder->in_width = in_width;
    encoder->in_height = in_height;
    encoder->out_width = out_width;
    encoder->out_height = out_height;
    encoder->codec_type = codec_type;
    encoder->gop_size = GOP_SIZE;
    encoder->frames_since_last_iframe = 0;
    enum AVPixelFormat in_format = AV_PIX_FMT_RGB32;
    enum AVPixelFormat hw_format = AV_PIX_FMT_QSV;
    enum AVPixelFormat sw_format = AV_PIX_FMT_RGB32;

    // init intake format in sw_frame

    encoder->sw_frame = av_frame_alloc();
    encoder->sw_frame->format = in_format;
    encoder->sw_frame->width = encoder->in_width;
    encoder->sw_frame->height = encoder->in_height;
    encoder->sw_frame->pts = 0;

    // set frame size and allocate memory for it
    int frame_size =
        av_image_get_buffer_size(in_format, encoder->out_width, encoder->out_height, 1);
    encoder->sw_frame_buffer = safe_malloc(frame_size);

    // fill picture with empty frame buffer
    av_image_fill_arrays(encoder->sw_frame->data, encoder->sw_frame->linesize,
                         (uint8_t *)encoder->sw_frame_buffer, in_format, encoder->out_width,
                         encoder->out_height, 1);

    // init hw_device_ctx
    if (av_hwdevice_ctx_create(&encoder->hw_device_ctx, AV_HWDEVICE_TYPE_QSV, NULL, NULL, 0) < 0) {
        LOG_WARNING("Failed to create hardware device context");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init encoder format in context

    if (encoder->codec_type == CODEC_TYPE_H264) {
        encoder->codec = avcodec_find_encoder_by_name("h264_qsv");
    } else if (encoder->codec_type == CODEC_TYPE_H265) {
        encoder->codec = avcodec_find_encoder_by_name("hevc_qsv");
    }

    encoder->context = avcodec_alloc_context3(encoder->codec);
    encoder->context->width = encoder->out_width;
    encoder->context->height = encoder->out_height;
    encoder->context->bit_rate = bitrate;
    encoder->context->rc_max_rate = bitrate;
    encoder->context->rc_buffer_size = 4 * (bitrate / FPS);
    encoder->context->time_base.num = 1;
    encoder->context->time_base.den = FPS;
    encoder->context->gop_size = encoder->gop_size;
    encoder->context->keyint_min = 5;
    encoder->context->pix_fmt = hw_format;

    // assign hw_device_ctx
    av_buffer_unref(&encoder->context->hw_frames_ctx);
    encoder->context->hw_frames_ctx = av_hwframe_ctx_alloc(encoder->hw_device_ctx);

    // init HWFramesContext
    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)encoder->context->hw_frames_ctx->data;
    frames_ctx->format = hw_format;
    frames_ctx->sw_format = sw_format;
    frames_ctx->width = encoder->in_width;
    frames_ctx->height = encoder->in_height;
    frames_ctx->initial_pool_size = 2;

    if (av_hwframe_ctx_init(encoder->context->hw_frames_ctx) < 0) {
        LOG_WARNING("Failed to initialize hardware frames context");
        destroy_video_encoder(encoder);
        return NULL;
    }

    if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
        LOG_WARNING("Failed to open context for stream");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init hardware frame
    encoder->hw_frame = av_frame_alloc();
    int res = av_hwframe_get_buffer(encoder->context->hw_frames_ctx, encoder->hw_frame, 0);
    if (res < 0) {
        LOG_WARNING("Failed to init buffer for video encoder hw frames: %s", av_err2str(res));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init resizing in filter_graph

    encoder->filter_graph = avfilter_graph_alloc();
    if (!encoder->filter_graph) {
        LOG_WARNING("Unable to create filter graph");
        destroy_video_encoder(encoder);
        return NULL;
    }

#define N_FILTERS_QSV 3
    // source -> scale_qsv -> sink
    const AVFilter *filters[N_FILTERS_QSV] = {0};
    filters[0] = avfilter_get_by_name("buffer");
    filters[1] = avfilter_get_by_name("scale_qsv");
    filters[2] = avfilter_get_by_name("buffersink");

    for (int i = 0; i < N_FILTERS_NVENC; ++i) {
        if (!filters[i]) {
            LOG_WARNING("Could not find filter %d in the list!", i);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    AVFilterContext *filter_contexts[N_FILTERS_QSV] = {0};

    // source buffer
    filter_contexts[0] = avfilter_graph_alloc_filter(encoder->filter_graph, filters[0], "src");
    AVBufferSrcParameters *avbsp = av_buffersrc_parameters_alloc();
    avbsp->width = encoder->in_width;
    avbsp->height = encoder->in_height;
    avbsp->format = hw_format;
    avbsp->frame_rate = (AVRational){FPS, 1};
    avbsp->time_base = (AVRational){1, FPS};
    avbsp->hw_frames_ctx = encoder->context->hw_frames_ctx;
    av_buffersrc_parameters_set(filter_contexts[0], avbsp);
    if (avfilter_init_str(filter_contexts[0], NULL) < 0) {
        LOG_WARNING("Unable to initialize buffer source");
        destroy_video_encoder(encoder);
        return NULL;
    }
    av_free(avbsp);
    encoder->filter_graph_source = filter_contexts[0];

    // scale_qsv (this is not tested yet, but should either just work or be easy
    // to fix on QSV-supporting machines)
    filter_contexts[1] =
        avfilter_graph_alloc_filter(encoder->filter_graph, filters[1], "scale_qsv");
    char options_string[60] = "";
    snprintf(options_string, 60, "w=%d:h=%d", encoder->out_width, encoder->out_height);
    if (avfilter_init_str(filter_contexts[1], options_string) < 0) {
        LOG_WARNING("Unable to initialize scale filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // sink buffer
    if (avfilter_graph_create_filter(&filter_contexts[2], filters[2], "sink", NULL, NULL,
                                     encoder->filter_graph) < 0) {
        LOG_WARNING("Unable to initialize buffer sink");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->filter_graph_sink = filter_contexts[2];

    // connect the filters in a simple line
    for (int i = 0; i < N_FILTERS_QSV - 1; ++i) {
        if (avfilter_link(filter_contexts[i], 0, filter_contexts[i + 1], 0) < 0) {
            LOG_WARNING("Unable to link filters %d to %d", i, i + 1);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    int err = avfilter_graph_config(encoder->filter_graph, NULL);
    if (err < 0) {
        LOG_WARNING("Unable to configure the filter graph: %s", av_err2str(err));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init transfer frame
    encoder->filtered_frame = av_frame_alloc();

    return encoder;
}

VideoEncoder *create_sw_encoder(int in_width, int in_height, int out_width, int out_height,
                                int bitrate, CodecType codec_type) {
    /*
        Create an FFmpeg software encoder.

        Arguments:
            in_width (int): Width of the frames that the encoder intakes
            in_height (int): height of the frames that the encoder intakes
            out_width (int): width of the frames that the encoder outputs
            out_height (int): Height of the frames that the encoder outputs
            bitrate (int): bits per second the encoder will encode to
            codec_type (CodecType): Codec (currently H264 or H265) the encoder will use

        Returns:
            (VideoEncoder*): the newly created encoder
     */
    LOG_INFO("Trying software encoder...");
    VideoEncoder *encoder = (VideoEncoder *)safe_malloc(sizeof(VideoEncoder));
    memset(encoder, 0, sizeof(VideoEncoder));

    encoder->type = SOFTWARE_ENCODE;
    encoder->in_width = in_width;
    encoder->in_height = in_height;
    if (out_width % 2) out_width = out_width + 1;
    if (out_height % 2) out_height = out_height + 1;
    encoder->out_width = out_width;
    encoder->out_height = out_height;
    encoder->codec_type = codec_type;
    encoder->gop_size = GOP_SIZE;
    encoder->frames_since_last_iframe = 0;

    enum AVPixelFormat in_format = AV_PIX_FMT_RGB32;
    enum AVPixelFormat out_format = AV_PIX_FMT_YUV420P;

    // init intake format in sw_frame

    encoder->sw_frame = av_frame_alloc();
    encoder->sw_frame->format = in_format;
    encoder->sw_frame->width = encoder->in_width;
    encoder->sw_frame->height = encoder->in_height;
    encoder->sw_frame->pts = 0;

    // set frame size and allocate memory for it
    int frame_size =
        av_image_get_buffer_size(out_format, encoder->out_width, encoder->out_height, 1);
    encoder->sw_frame_buffer = safe_malloc(frame_size);

    // fill picture with empty frame buffer
    av_image_fill_arrays(encoder->sw_frame->data, encoder->sw_frame->linesize,
                         (uint8_t *)encoder->sw_frame_buffer, out_format, encoder->out_width,
                         encoder->out_height, 1);

    // init resizing and resampling in filter_graph

    encoder->filter_graph = avfilter_graph_alloc();
    if (!encoder->filter_graph) {
        LOG_WARNING("Unable to create filter graph");
        destroy_video_encoder(encoder);
        return NULL;
    }

#define N_FILTERS_SW 4
    // source -> format -> scale -> sink
    const AVFilter *filters[N_FILTERS_SW] = {0};
    filters[0] = avfilter_get_by_name("buffer");
    filters[1] = avfilter_get_by_name("format");
    filters[2] = avfilter_get_by_name("scale");
    filters[3] = avfilter_get_by_name("buffersink");

    for (int i = 0; i < N_FILTERS_SW; ++i) {
        if (!filters[i]) {
            LOG_WARNING("Could not find filter %d in the list!", i);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    AVFilterContext *filter_contexts[N_FILTERS_SW] = {0};

    // source buffer
    filter_contexts[0] = avfilter_graph_alloc_filter(encoder->filter_graph, filters[0], "src");
    av_opt_set_int(filter_contexts[0], "width", encoder->in_width, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(filter_contexts[0], "height", encoder->in_height, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(filter_contexts[0], "pix_fmt", av_get_pix_fmt_name(in_format),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(filter_contexts[0], "time_base", (AVRational){1, FPS}, AV_OPT_SEARCH_CHILDREN);
    if (avfilter_init_str(filter_contexts[0], NULL) < 0) {
        LOG_WARNING("Unable to initialize buffer source");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->filter_graph_source = filter_contexts[0];

    // format
    filter_contexts[1] = avfilter_graph_alloc_filter(encoder->filter_graph, filters[1], "format");
    av_opt_set(filter_contexts[1], "pix_fmts", av_get_pix_fmt_name(out_format),
               AV_OPT_SEARCH_CHILDREN);
    if (avfilter_init_str(filter_contexts[1], NULL) < 0) {
        LOG_WARNING("Unable to initialize format filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // scale
    filter_contexts[2] = avfilter_graph_alloc_filter(encoder->filter_graph, filters[2], "scale");
    char options_string[60] = "";
    snprintf(options_string, 60, "w=%d:h=%d", encoder->out_width, encoder->out_height);
    if (avfilter_init_str(filter_contexts[2], options_string) < 0) {
        LOG_WARNING("Unable to initialize scale filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // sink buffer
    if (avfilter_graph_create_filter(&filter_contexts[3], filters[3], "sink", NULL, NULL,
                                     encoder->filter_graph) < 0) {
        LOG_WARNING("Unable to initialize buffer sink");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->filter_graph_sink = filter_contexts[3];

    // connect the filters in a simple line
    for (int i = 0; i < N_FILTERS_SW - 1; ++i) {
        if (avfilter_link(filter_contexts[i], 0, filter_contexts[i + 1], 0) < 0) {
            LOG_WARNING("Unable to link filters %d to %d", i, i + 1);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    // configure the graph
    int err = avfilter_graph_config(encoder->filter_graph, NULL);
    if (err < 0) {
        LOG_WARNING("Unable to configure the filter graph: %s", av_err2str(err));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init transfer frame
    encoder->filtered_frame = av_frame_alloc();

    // init encoder format in context

    if (encoder->codec_type == CODEC_TYPE_H264) {
        encoder->codec = avcodec_find_encoder_by_name("libx264");
    } else if (encoder->codec_type == CODEC_TYPE_H265) {
        encoder->codec = avcodec_find_encoder_by_name("libx265");
    }

    encoder->context = avcodec_alloc_context3(encoder->codec);
    encoder->context->width = encoder->out_width;
    encoder->context->height = encoder->out_height;
    encoder->context->bit_rate = bitrate;
    encoder->context->rc_max_rate = bitrate;
    encoder->context->rc_buffer_size = 4 * (bitrate / FPS);
    encoder->context->time_base.num = 1;
    encoder->context->time_base.den = FPS;
    encoder->context->gop_size = encoder->gop_size;
    encoder->context->keyint_min = 5;
    encoder->context->pix_fmt = out_format;
    encoder->context->max_b_frames = 0;

    set_opt(encoder, "preset", "fast");
    set_opt(encoder, "tune", "zerolatency");

    if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
        LOG_WARNING("Failed to open context for stream");
        destroy_video_encoder(encoder);
        return NULL;
    }

    return encoder;
}

/*
============================
Public Function Implementations
============================
*/

VideoEncoder *create_video_encoder(int in_width, int in_height, int out_width, int out_height,
                                   int bitrate, CodecType codec_type) {
    /*
        Create an FFmpeg encoder with the specified parameters. First try NVENC hardware encoding,
       then software encoding if that fails.

        Arguments:
            in_width (int): Width of the frames that the encoder intakes
            in_height (int): height of the frames that the encoder intakes
            out_width (int): width of the frames that the encoder outputs
            out_height (int): Height of the frames that the encoder outputs
            bitrate (int): bits per second the encoder will encode to
            codec_type (CodecType): Codec (currently H264 or H265) the encoder will use

        Returns:
            (VideoEncoder*): the newly created encoder
     */

    // setup the AVCodec and AVFormatContext
#if !USING_SERVERSIDE_SCALE
    out_width = in_width;
    out_height = in_height;
#endif

    // Try our nvidia encoder first
#if USING_NVIDIA_CAPTURE_AND_ENCODE
#if USING_SERVERSIDE_SCALE
    LOG_ERROR(
        "Cannot create nvidia encoder, does not accept in_width and in_height when using "
        "serverside scaling");
#else
    NvidiaEncoder *nvidia_encoder =
        create_nvidia_encoder(bitrate, codec_type, out_width, out_height);
    if (!nvidia_encoder) {
        LOG_ERROR("Failed to create nvidia encoder!");
    }
#endif
#endif

    VideoEncoderCreator encoder_precedence[] = {create_nvenc_encoder, create_sw_encoder};
    VideoEncoder *encoder = NULL;
    for (unsigned int i = 0; i < sizeof(encoder_precedence) / sizeof(VideoEncoderCreator); ++i) {
        encoder =
            encoder_precedence[i](in_width, in_height, out_width, out_height, bitrate, codec_type);

        if (!encoder) {
            LOG_WARNING("Video encoder: Failed, trying next encoder");
            encoder = NULL;
        } else {
            LOG_INFO("Video encoder: Success!");
            break;
        }
    }

    if (!encoder) {
        LOG_ERROR("Video encoder: All encoders failed!");
        return NULL;
    }

    encoder->nvidia_encoder = nvidia_encoder;

    return encoder;
}

int video_encoder_frame_intake(VideoEncoder *encoder, void *rgb_pixels, int pitch) {
    /*
        Copy frame data in rgb_pixels and pitch to the software frame, and to the hardware frame if
       possible.

        Arguments:
            encoder (VideoEncoder*): video encoder containing encoded frames
            rgb_pixels (void*): pixel data for the frame
            pitch (int): Pitch data for the frame

        Returns:
            (int): 0 on success, -1 on failure
     */
    memset(encoder->sw_frame->data, 0, sizeof(encoder->sw_frame->data));
    memset(encoder->sw_frame->linesize, 0, sizeof(encoder->sw_frame->linesize));
    encoder->sw_frame->data[0] = (uint8_t *)rgb_pixels;
    encoder->sw_frame->linesize[0] = pitch;
    encoder->sw_frame->pts++;

    if (encoder->hw_frame) {
        int res = av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);
        if (res < 0) {
            LOG_ERROR("Unable to transfer frame to hardware frame: %s", av_err2str(res));
            return -1;
        }
        encoder->hw_frame->pict_type = encoder->sw_frame->pict_type;
    }
    return 0;
}

int video_encoder_encode(VideoEncoder *encoder) {
    /*
        Encode a frame using the encoder and store the resulting packets into encoder->packets. If
       the frame has alreday been encoded, this function does nothing.

        Arguments:
            encoder (VideoEncoder*): encoder that will encode the frame

        Returns:
            (int): 0 on success, -1 on failure
     */
    if (encoder->capture_is_on_nvidia) {
        nvidia_encoder_encode(encoder->nvidia_encoder);

        // Set meta data
        encoder->is_iframe = encoder->nvidia_encoder->is_iframe;
        encoder->out_width = encoder->nvidia_encoder->width;
        encoder->out_height = encoder->nvidia_encoder->height;
        encoder->codec_type = encoder->nvidia_encoder->codec_type;

        // Construct frame packets
        encoder->encoded_frame_size = encoder->nvidia_encoder->frame_size;
        encoder->num_packets = 1;
        encoder->packets[0].data = encoder->nvidia_encoder->frame;
        encoder->packets[0].size = encoder->nvidia_encoder->frame_size;
        encoder->encoded_frame_size += 8;

        return 0;
    }

    if (video_encoder_send_frame(encoder)) {
        LOG_ERROR("Unable to send frame to encoder!");
        return -1;
    }

    // If the encoder has been used to encode a frame before we should clear packet data for
    // previously used packets
    if (encoder->num_packets) {
        for (int i = 0; i < encoder->num_packets; i++) {
            av_packet_unref(&encoder->packets[i]);
        }
    }

    // The encoded frame size starts out as sizeof(int) because of the way we send packets. See
    // encode.h and decode.h.
    encoder->encoded_frame_size = sizeof(int);
    encoder->num_packets = 0;
    int res;

    // receive packets until we receive a nonzero code (indicating either an encoding error, or that
    // all packets have been received).
    while ((res = video_encoder_receive_packet(encoder, &encoder->packets[encoder->num_packets])) ==
           0) {
        encoder->encoded_frame_size += 4 + encoder->packets[encoder->num_packets].size;
        encoder->num_packets++;
        if (encoder->num_packets == MAX_ENCODER_PACKETS) {
            LOG_ERROR("TOO MANY PACKETS: REACHED %d", encoder->num_packets);
            return -1;
        }
    }
    if (res < 0) {
        return -1;
    }

    // set iframe metadata
    encoder->is_iframe = encoder->frames_since_last_iframe % encoder->gop_size == 0;
    if (encoder->is_iframe) {
        encoder->frames_since_last_iframe = 0;
    }

    encoder->frames_since_last_iframe++;
    return 0;
}

void video_encoder_write_buffer(VideoEncoder *encoder, int *buf) {
    /*
        Write all the encoded packets found in encoder into buf, which we assume is large enough to
       hold the data.

        Arguments:
            encoder (VideoEncoder*): encoder holding encoded packets
            buf (int*): memory buffer for storing the packets
    */
    *buf = encoder->num_packets;
    buf++;
    for (int i = 0; i < encoder->num_packets; i++) {
        *buf = encoder->packets[i].size;
        buf++;
    }
    char *char_buf = (void *)buf;
    for (int i = 0; i < encoder->num_packets; i++) {
        memcpy(char_buf, encoder->packets[i].data, encoder->packets[i].size);
        char_buf += encoder->packets[i].size;
    }
}

void video_encoder_set_iframe(VideoEncoder *encoder) {
    /*
        Set the next frame to be an iframe.

        Arguments:
            encoder (VideoEncoder*): Encoder containing the frame
    */
    if (encoder->capture_is_on_nvidia) {
        return;
    }
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_I;
    encoder->sw_frame->pts +=
        encoder->context->gop_size - (encoder->sw_frame->pts % encoder->context->gop_size);
    encoder->sw_frame->key_frame = 1;
}

void video_encoder_unset_iframe(VideoEncoder *encoder) {
    /*
        Indicate that the next frame is not an iframe.

        Arguments:
            encoder (VideoEncoder*): encoder containing the frame
    */
    if (encoder->capture_is_on_nvidia) {
        return;
    }
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_NONE;
    encoder->sw_frame->key_frame = 0;
}

void destroy_video_encoder(VideoEncoder *encoder) {
    /*
        Destroy all components of the encoder, then free the encoder itself.

        Arguments:
            encoder (VideoEncoder*): encoder to destroy
    */
    LOG_INFO("Destroying video encoder...");

    // Destroy the nvidia encoder, if any
    if (encoder->nvidia_encoder) {
        destroy_nvidia_encoder(encoder->nvidia_encoder);
    }

    // check if encoder encoder exists
    if (encoder == NULL) {
        LOG_INFO("Encoder empty, not destroying anything.");
        return;
    }

    if (encoder->context) {
        avcodec_free_context(&encoder->context);
    }

    if (encoder->filter_graph) {
        avfilter_graph_free(&encoder->filter_graph);
    }

    if (encoder->hw_device_ctx) {
        av_buffer_unref(&encoder->hw_device_ctx);
    }

    // free packets
    for (int i = 0; i < MAX_ENCODER_PACKETS; i++) {
        av_packet_unref(&encoder->packets[i]);
    }

    av_frame_free(&encoder->hw_frame);
    av_frame_free(&encoder->sw_frame);
    av_frame_free(&encoder->filtered_frame);

    // free the buffer and encoder
    free(encoder->sw_frame_buffer);
    free(encoder);
    return;
}

// Goes through NVENC/QSV/SOFTWARE and sees which one works, cascading to the
// next one when the previous one doesn't work
int video_encoder_send_frame(VideoEncoder *encoder) {
    /*
        Send a frame through the filter graph, then encode it.

        Arguments:
            encoder (VideoEncoder*): encoder used to encode the frame

        Returns:
            (int): 0 on success, -1 on failure
    */
    AVFrame *active_frame = encoder->sw_frame;
    if (encoder->hw_frame) {
        active_frame = encoder->hw_frame;
    }

    int res = av_buffersrc_add_frame(encoder->filter_graph_source, active_frame);
    if (res < 0) {
        LOG_WARNING("Error submitting frame to the filter graph: %s", av_err2str(res));
    }

    if (encoder->hw_frame) {
        // have to re-create buffers after sending to filter graph
        av_hwframe_get_buffer(encoder->context->hw_frames_ctx, encoder->hw_frame, 0);
    }

    int res_buffer;

    // submit all available frames to the encoder
    while ((res_buffer = av_buffersink_get_frame(encoder->filter_graph_sink,
                                                 encoder->filtered_frame)) >= 0) {
        int res_encoder = avcodec_send_frame(encoder->context, encoder->filtered_frame);

        // unref the frame so it may be reused
        av_frame_unref(encoder->filtered_frame);

        if (res_encoder < 0) {
            LOG_WARNING("Error sending frame for encoding: %s", av_err2str(res_encoder));
            return -1;
        }
    }
    if (res_buffer < 0 && res_buffer != AVERROR(EAGAIN) && res_buffer != AVERROR_EOF) {
        LOG_WARNING("Error getting frame from the filter graph: %d -- %s", res_buffer,
                    av_err2str(res_buffer));
        return -1;
    }

    return 0;
}

int video_encoder_receive_packet(VideoEncoder *encoder, AVPacket *packet) {
    /*
        Wrapper around FFmpeg's avcodec_receive_packet. Get an encoded packet from the encoder and
       store it in packet.

        Arguments:
            encoder (VideoEncoder*): encoder used to encode the frame
            packet (AVPacket*): packet in which to store encoded data

        Returns:
            (int): 1 on EAGAIN (no more packets), 0 on success (call this function again), and -1 on
       failure.
    */
    int res_encoder;

    // receive_packet already calls av_packet_unref, no need to reinitialize packet
    res_encoder = avcodec_receive_packet(encoder->context, packet);
    if (res_encoder == AVERROR(EAGAIN) || res_encoder == AVERROR(EOF)) {
        return 1;
    } else if (res_encoder < 0) {
        LOG_ERROR("Error getting frame from the encoder: %s", av_err2str(res_encoder));
        return -1;
    }

    return 0;
}
