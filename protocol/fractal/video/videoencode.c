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

// TODO: all this creation stuff goes in ffmpeg encode
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

    VideoEncoder *encoder = malloc(sizeof(VideoEncoder));
    memset(encoder, 0, sizeof(VideoEncoder));
    encoder->codec_type = codec_type;

#if USING_NVIDIA_CAPTURE_AND_ENCODE
#if USING_SERVERSIDE_SCALE
    LOG_ERROR(
        "Cannot create nvidia encoder, does not accept in_width and in_height when using "
        "serverside scaling");
#else
    encoder->nvidia_encoder = create_nvidia_encoder(bitrate, codec_type, out_width, out_height);
    if (!encoder->nvidia_encoder) {
        LOG_ERROR("Failed to create nvidia encoder!");
        encoder->active_encoder = FFMPEG_ENCODER;
    } else {
        encoder->active_encoder = NVIDIA_ENCODER;
    }
#endif  // USING_SERVERSIDE_SCALE
#else
    encoder->active_encoder = FFMPEG_ENCODER;
#endif  // USING_NVIDIA_CAPTURE_AND_ENCODE

    encoder->ffmpeg_encoder =
        create_ffmpeg_encoder(in_width, in_height, out_width, out_height, bitrate, codec_type);
    if (!encoder->ffmpeg_encoder) {
        LOG_ERROR("FFmpeg encoder creation failed!");
        return NULL;
    }

    return encoder;
}

void transfer_nvidia_data(VideoEncoder *encoder) {
    if (encoder->active_encoder != NVIDIA_ENCODER) {
        LOG_ERROR("Encoder is not using nvidia, but we are trying to call transfer_nvidia_data!");
        return;
    }
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
}

int transfer_ffmpeg_data(VideoEncoder *encoder) {
    if (encoder->active_encoder != FFMPEG_ENCODER) {
        LOG_ERROR("Encoder is not using ffmpeg, but we are trying to call transfer_ffmpge_data!");
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
    while ((res = ffmpeg_encode_receive_packet(encoder, &encoder->packets[encoder->num_packets])) ==
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
    if (!encoder) {
        LOG_ERROR("Tried to video_encoder_encode with a NULL encoder!");
        return -1;
    }
    switch (encoder->active_encoder) {
        case NVIDIA_ENCODER:
#ifdef __linux__
            nvidia_encoder_encode(encoder->nvidia_encoder);
            transfer_nvidia_data(encoder);
            return 0;
#else
            LOG_ERROR("Cannot use nvidia encoder if not on Linux!");
            return -1;
#endif
        case FFMPEG_ENCODER:
            if (ffmpeg_encoder_send_frame(encoder)) {
                LOG_ERROR("Unable to send frame to encoder!");
                return -1;
            }
            return transfer_ffmpeg_data(encoder);
        default:
            LOG_ERROR("Unknown encoder type: %d!", encoder->active_encoder);
            return -1;
    }
}

bool reconfigure_encoder(VideoEncoder *encoder, int width, int height, int bitrate,
                         CodecType codec) {
    if (!encoder) {
        LOG_ERROR("Calling reconfigure_encoder on a NULL encoder!");
        return false;
    }
    switch (encoder->active_encoder) {
        case NVIDIA_ENCODER:
#ifdef __linux__
            return nvidia_reconfigure_encoder(encoder->nvidia_encoder, width, height, bitrate,
                                              codec);
#else
            return false;
#endif
        case FFMPEG_ENCODER:
            // Haven't implemented ffmpeg reconfiguring yet
            return false;
        default:
            LOG_ERROR("Unknown encoder type: %d!", encoder->active_encoder);
            return -1;
    }
}

void video_encoder_set_iframe(VideoEncoder *encoder) {
    /*
        Set the next frame to be an IDR-frame with SPS/PPS headers.

        Arguments:
            encoder (VideoEncoder*): Encoder containing the frame
    */
    if (!encoder) {
        LOG_ERROR("video_encoder_set_iframe received NULL encoder!");
        return;
    }
    switch (encoder->active_encoder) {
        case NVIDIA_ENCODER:
#ifdef __linux__
            nvidia_set_iframe(encoder->nvidia_encoder);
#else
            LOG_ERROR("nvidia set-iframe is not implemented on Windows!");
#endif
        case FFMPEG_ENCODER:
            ffmpeg_set_iframe(encoder->ffmpeg_encoder);
        default:
            LOG_ERROR("Unknown encoder type: %d!", encoder->active_encoder);
            return -1;
    }
}

void video_encoder_unset_iframe(VideoEncoder *encoder) {
    /*
        Indicate that the next frame is not an iframe.

        Arguments:
            encoder (VideoEncoder*): encoder containing the frame
    */
    if (!encoder) {
        LOG_ERROR("video_encoder_set_iframe received NULL encoder!");
        return;
    }
    switch (encoder->active_encoder) {
        case NVIDIA_ENCODER:
#ifdef __linux__
            nvidia_unset_iframe(encoder->nvidia_encoder);
#else
            LOG_ERROR("nvidia set-iframe is not implemented on Windows!");
#endif
        case FFMPEG_ENCODER:
            ffmpeg_unset_iframe(encoder->ffmpeg_encoder);
        default:
            LOG_ERROR("Unknown encoder type: %d!", encoder->active_encoder);
            return -1;
    }
}

void destroy_video_encoder(VideoEncoder *encoder) {
    /*
        Destroy all components of the encoder, then free the encoder itself.

        Arguments:
            encoder (VideoEncoder*): encoder to destroy
    */
    LOG_INFO("Destroying video encoder...");

    // Check if encoder exists
    if (encoder == NULL) {
        LOG_INFO("Encoder empty, not destroying anything.");
        return;
    }

#ifdef __linux__
    // Destroy the nvidia encoder, if any
    if (encoder->nvidia_encoder) {
        destroy_nvidia_encoder(encoder->nvidia_encoder);
    }
#endif
    if (encoder->ffmpeg_encoder) {
        destroy_ffmpeg_encoder(encoder->ffmpeg_encoder);
    }

    // free packets
    for (int i = 0; i < MAX_ENCODER_PACKETS; i++) {
        av_packet_unref(&encoder->packets[i]);
    }
    free(encoder);
}
