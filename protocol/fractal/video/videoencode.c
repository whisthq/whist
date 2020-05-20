/*
 * Video encoding via FFmpeg library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "videoencode.h"

#include <libavfilter/avfilter.h>
#include <stdio.h>
#include <stdlib.h>

#define GOP_SIZE 9999

void set_opt(video_encoder_t *encoder, char *option, char *value) {
    int ret = av_opt_set(encoder->pCodecCtx->priv_data, option, value, 0);
    if (ret < 0) {
        LOG_WARNING("Could not av_opt_set %s to %s!", option, value);
    }
}

typedef video_encoder_t *(*video_encoder_creator)(int, int, int, int, int);

video_encoder_t *create_nvenc_encoder(int in_width, int in_height,
                                      int out_width, int out_height,
                                      int bitrate) {
    LOG_INFO("Trying NVENC encoder...");
    video_encoder_t *encoder =
        (video_encoder_t *)malloc(sizeof(video_encoder_t));
    memset(encoder, 0, sizeof(video_encoder_t));

    encoder->in_width = in_width;
    encoder->in_height = in_height;
    encoder->out_width = out_width;
    encoder->out_height = out_height;
    encoder->gop_size = GOP_SIZE;
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
    int frame_size = av_image_get_buffer_size(in_format, encoder->out_width,
                                              encoder->out_height, 1);
    encoder->sw_frame_buffer = malloc(frame_size);

    // fill picture with empty frame buffer
    av_image_fill_arrays(encoder->sw_frame->data, encoder->sw_frame->linesize,
                         (uint8_t *)encoder->sw_frame_buffer, in_format,
                         encoder->out_width, encoder->out_height, 1);

    // init hw_device_ctx
    if (av_hwdevice_ctx_create(&encoder->hw_device_ctx, AV_HWDEVICE_TYPE_CUDA,
                               "CUDA", NULL, 0) < 0) {
        LOG_WARNING("Failed to create hardware device context");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init encoder format in pCodecCtx

    encoder->pCodec = avcodec_find_encoder_by_name("h264_nvenc");

    encoder->pCodecCtx = avcodec_alloc_context3(encoder->pCodec);
    encoder->pCodecCtx->width = encoder->out_width;
    encoder->pCodecCtx->height = encoder->out_height;
    encoder->pCodecCtx->bit_rate = bitrate;
    encoder->pCodecCtx->rc_max_rate = bitrate;
    encoder->pCodecCtx->rc_buffer_size = 4 * (bitrate / FPS);
    encoder->pCodecCtx->time_base.num = 1;
    encoder->pCodecCtx->time_base.den = FPS;
    encoder->pCodecCtx->gop_size = encoder->gop_size;
    encoder->pCodecCtx->keyint_min = 5;
    encoder->pCodecCtx->pix_fmt = hw_format;

    set_opt(encoder, "nonref_p", "1");
    set_opt(encoder, "preset", "llhp");
    set_opt(encoder, "rc", "cbr_ld_hq");
    set_opt(encoder, "zerolatency", "1");
    set_opt(encoder, "delay", "0");

    // assign hw_device_ctx
    av_buffer_unref(&encoder->pCodecCtx->hw_frames_ctx);
    encoder->pCodecCtx->hw_frames_ctx =
        av_hwframe_ctx_alloc(encoder->hw_device_ctx);

    // init HWFramesContext
    AVHWFramesContext *frames_ctx =
        (AVHWFramesContext *)encoder->pCodecCtx->hw_frames_ctx->data;
    frames_ctx->format = hw_format;
    frames_ctx->sw_format = sw_format;
    frames_ctx->width = encoder->in_width;
    frames_ctx->height = encoder->in_height;
    if (av_hwframe_ctx_init(encoder->pCodecCtx->hw_frames_ctx) < 0) {
        LOG_WARNING("Failed to initialize hardware frames context");
        destroy_video_encoder(encoder);
        return NULL;
    }

    if (avcodec_open2(encoder->pCodecCtx, encoder->pCodec, NULL) < 0) {
        LOG_WARNING("Failed to open context for stream");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init hardware frame
    encoder->hw_frame = av_frame_alloc();
    int res = av_hwframe_get_buffer(encoder->pCodecCtx->hw_frames_ctx,
                                    encoder->hw_frame, 0);
    if (res < 0) {
        LOG_WARNING("Failed to init buffer for video encoder hw frames: %s",
                    av_err2str(res));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init resizing in pFilterGraph

    encoder->pFilterGraph = avfilter_graph_alloc();
    if (!encoder->pFilterGraph) {
        LOG_WARNING("Unable to create filter graph");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // source -> scale_cuda -> sink
    const AVFilter *filters[3] = {0};
    filters[0] = avfilter_get_by_name("buffer");
    filters[1] = avfilter_get_by_name("scale_cuda");
    filters[2] = avfilter_get_by_name("buffersink");

    AVFilterContext *filter_contexts[3] = {0};
    int n_filters = 3;

    // source buffer
    filter_contexts[0] =
        avfilter_graph_alloc_filter(encoder->pFilterGraph, filters[0], "src");
    AVBufferSrcParameters *avbsp = av_buffersrc_parameters_alloc();
    avbsp->width = encoder->in_width;
    avbsp->height = encoder->in_height;
    avbsp->format = hw_format;
    avbsp->frame_rate = (AVRational){FPS, 1};
    avbsp->time_base = (AVRational){1, FPS};
    avbsp->hw_frames_ctx = encoder->pCodecCtx->hw_frames_ctx;
    av_buffersrc_parameters_set(filter_contexts[0], avbsp);
    if (avfilter_init_str(filter_contexts[0], NULL) < 0) {
        LOG_WARNING("Unable to initialize buffer source");
        destroy_video_encoder(encoder);
        return NULL;
    }
    av_free(avbsp);
    encoder->pFilterGraphSource = filter_contexts[0];

    // scale_cuda
    filter_contexts[1] = avfilter_graph_alloc_filter(encoder->pFilterGraph,
                                                     filters[1], "scale_cuda");
    char options_string[60] = "";
    snprintf(options_string, 60, "w=%d:h=%d", encoder->out_width,
             encoder->out_height);
    if (avfilter_init_str(filter_contexts[1], options_string) < 0) {
        LOG_WARNING("Unable to initialize scale filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // sink buffer
    if (avfilter_graph_create_filter(&filter_contexts[2], filters[2], "sink",
                                     NULL, NULL, encoder->pFilterGraph) < 0) {
        LOG_WARNING("Unable to initialize buffer sink");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->pFilterGraphSink = filter_contexts[2];

    // connect the filters in a simple line
    for (int i = 0; i < n_filters - 1; ++i) {
        if (avfilter_link(filter_contexts[i], 0, filter_contexts[i + 1], 0) <
            0) {
            LOG_WARNING("Unable to link filters %d to %d", i, i + 1);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    int err = avfilter_graph_config(encoder->pFilterGraph, NULL);
    if (err < 0) {
        LOG_WARNING("Unable to configure the filter graph: %s",
                    av_err2str(err));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init transfer frame
    encoder->filtered_frame = av_frame_alloc();

    return encoder;
}

video_encoder_t *create_qsv_encoder(int in_width, int in_height, int out_width,
                                    int out_height, int bitrate) {
    LOG_INFO("Trying QSV encoder...");
    in_width, in_height, out_width, out_height, bitrate;
    return NULL;
}

video_encoder_t *create_sw_encoder(int in_width, int in_height, int out_width,
                                   int out_height, int bitrate) {
    LOG_INFO("Trying software encoder...");
    video_encoder_t *encoder =
        (video_encoder_t *)malloc(sizeof(video_encoder_t));
    memset(encoder, 0, sizeof(video_encoder_t));

    encoder->in_width = in_width;
    encoder->in_height = in_height;
    encoder->out_width = out_width;
    encoder->out_height = out_height;
    encoder->gop_size = GOP_SIZE;
    enum AVPixelFormat in_format = AV_PIX_FMT_RGB32;
    enum AVPixelFormat out_format = AV_PIX_FMT_YUV420P;

    // init intake format in sw_frame

    encoder->sw_frame = av_frame_alloc();
    encoder->sw_frame->format = in_format;
    encoder->sw_frame->width = encoder->in_width;
    encoder->sw_frame->height = encoder->in_height;
    encoder->sw_frame->pts = 0;

    // set frame size and allocate memory for it
    int frame_size = av_image_get_buffer_size(out_format, encoder->out_width,
                                              encoder->out_height, 1);
    encoder->sw_frame_buffer = malloc(frame_size);

    // fill picture with empty frame buffer
    av_image_fill_arrays(encoder->sw_frame->data, encoder->sw_frame->linesize,
                         (uint8_t *)encoder->sw_frame_buffer, out_format,
                         encoder->out_width, encoder->out_height, 1);

    // init resizing and resampling in pFilterGraph

    encoder->pFilterGraph = avfilter_graph_alloc();
    if (!encoder->pFilterGraph) {
        LOG_WARNING("Unable to create filter graph");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // source -> format -> scale -> sink
    const AVFilter *filters[4] = {0};
    filters[0] = avfilter_get_by_name("buffer");
    filters[1] = avfilter_get_by_name("format");
    filters[2] = avfilter_get_by_name("scale");
    filters[3] = avfilter_get_by_name("buffersink");

    AVFilterContext *filter_contexts[4] = {0};
    int n_filters = 4;

    // source buffer
    filter_contexts[0] =
        avfilter_graph_alloc_filter(encoder->pFilterGraph, filters[0], "src");
    av_opt_set_int(filter_contexts[0], "width", encoder->in_width,
                   AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(filter_contexts[0], "height", encoder->in_height,
                   AV_OPT_SEARCH_CHILDREN);
    av_opt_set(filter_contexts[0], "pix_fmt", av_get_pix_fmt_name(in_format),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(filter_contexts[0], "time_base", (AVRational){1, FPS},
                 AV_OPT_SEARCH_CHILDREN);
    if (avfilter_init_str(filter_contexts[0], NULL) < 0) {
        LOG_WARNING("Unable to initialize buffer source");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->pFilterGraphSource = filter_contexts[0];

    // format
    filter_contexts[1] = avfilter_graph_alloc_filter(encoder->pFilterGraph,
                                                     filters[1], "format");
    av_opt_set(filter_contexts[1], "pix_fmts", av_get_pix_fmt_name(out_format),
               AV_OPT_SEARCH_CHILDREN);
    if (avfilter_init_str(filter_contexts[1], NULL) < 0) {
        LOG_WARNING("Unable to initialize format filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // scale
    filter_contexts[2] =
        avfilter_graph_alloc_filter(encoder->pFilterGraph, filters[2], "scale");
    char options_string[60] = "";
    snprintf(options_string, 60, "w=%d:h=%d", encoder->out_width,
             encoder->out_height);
    if (avfilter_init_str(filter_contexts[2], options_string) < 0) {
        LOG_WARNING("Unable to initialize scale filter");
        destroy_video_encoder(encoder);
        return NULL;
    }

    // sink buffer
    if (avfilter_graph_create_filter(&filter_contexts[3], filters[3], "sink",
                                     NULL, NULL, encoder->pFilterGraph) < 0) {
        LOG_WARNING("Unable to initialize buffer sink");
        destroy_video_encoder(encoder);
        return NULL;
    }
    encoder->pFilterGraphSink = filter_contexts[3];

    // connect the filters in a simple line
    for (int i = 0; i < n_filters - 1; ++i) {
        if (avfilter_link(filter_contexts[i], 0, filter_contexts[i + 1], 0) <
            0) {
            LOG_WARNING("Unable to link filters %d to %d", i, i + 1);
            destroy_video_encoder(encoder);
            return NULL;
        }
    }

    // configure the graph
    int err = avfilter_graph_config(encoder->pFilterGraph, NULL);
    if (err < 0) {
        LOG_WARNING("Unable to configure the filter graph: %s",
                    av_err2str(err));
        destroy_video_encoder(encoder);
        return NULL;
    }

    // init transfer frame
    encoder->filtered_frame = av_frame_alloc();

    // init encoder format in pCodecCtx

    encoder->pCodec = avcodec_find_encoder_by_name("libx264");

    encoder->pCodecCtx = avcodec_alloc_context3(encoder->pCodec);
    encoder->pCodecCtx->width = encoder->out_width;
    encoder->pCodecCtx->height = encoder->out_height;
    encoder->pCodecCtx->bit_rate = bitrate;
    encoder->pCodecCtx->rc_max_rate = bitrate;
    encoder->pCodecCtx->rc_buffer_size = 4 * (bitrate / FPS);
    encoder->pCodecCtx->time_base.num = 1;
    encoder->pCodecCtx->time_base.den = FPS;
    encoder->pCodecCtx->gop_size = encoder->gop_size;
    encoder->pCodecCtx->keyint_min = 5;
    encoder->pCodecCtx->pix_fmt = out_format;
    encoder->pCodecCtx->max_b_frames = 0;

    set_opt(encoder, "preset", "fast");
    set_opt(encoder, "tune", "zerolatency");

    if (avcodec_open2(encoder->pCodecCtx, encoder->pCodec, NULL) < 0) {
        LOG_WARNING("Failed to open context for stream");
        destroy_video_encoder(encoder);
        return NULL;
    }

    return encoder;
}

// Goes through NVENC/QSV/SOFTWARE and sees which one works, cascading to the
// next one when the previous one doesn't work
video_encoder_t *create_video_encoder(int in_width, int in_height,
                                      int out_width, int out_height,
                                      int bitrate) {
    // setup the AVCodec and AVFormatContext
    // avcodec_register_all is deprecated on FFmpeg 4+
    // only linux uses FFmpeg 3.4.x because of canonical system packages
#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
#endif

#if !USING_SERVERSIDE_SCALE
    out_width = in_width;
    out_height = in_height;
#endif

    video_encoder_creator encoder_precedence[] = {
        create_nvenc_encoder, create_qsv_encoder, create_sw_encoder};
    video_encoder_t *encoder;
    for (unsigned int i = 0;
         i < sizeof(encoder_precedence) / sizeof(video_encoder_creator); ++i) {
        encoder = encoder_precedence[i](in_width, in_height, out_width,
                                        out_height, bitrate);
        if (!encoder) {
            LOG_WARNING("Video encoder: Failed, trying next encoder");
        } else {
            LOG_INFO("Video encoder: Success!");
            return encoder;
        }
    }

    LOG_ERROR("Video encoder: All encoders failed!");
    return NULL;
}

void destroy_video_encoder(video_encoder_t *encoder) {
    // check if encoder encoder exists
    if (encoder == NULL) {
        LOG_INFO("Encoder empty, not destroying anything.");
        return;
    }

    if (encoder->pCodecCtx) {
        avcodec_free_context(&encoder->pCodecCtx);
    }

    if (encoder->pFilterGraph) {
        avfilter_graph_free(&encoder->pFilterGraph);
    }

    if (encoder->hw_device_ctx) {
        av_buffer_unref(&encoder->hw_device_ctx);
    }

    av_frame_free(&encoder->hw_frame);
    av_frame_free(&encoder->sw_frame);
    av_frame_free(&encoder->filtered_frame);

    av_packet_unref(&encoder->packet);

    // free the buffer and encoder
    free(encoder->sw_frame_buffer);
    free(encoder);
    return;
}

void video_encoder_set_iframe(video_encoder_t *encoder) {
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_I;
    encoder->sw_frame->pts +=
        encoder->pCodecCtx->gop_size -
        (encoder->sw_frame->pts % encoder->pCodecCtx->gop_size);
    encoder->sw_frame->key_frame = 1;
}

void video_encoder_unset_iframe(video_encoder_t *encoder) {
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_NONE;
    encoder->sw_frame->key_frame = 0;
}

void video_encoder_filter_graph_intake(video_encoder_t *encoder,
                                       void *rgb_pixels) {
    if (false /*encoder->pSwsContext*/) {
        // uint8_t *in_data[1];
        // in_data[0] = (uint8_t *)rgb_pixels;
        // int in_linesize[1];
        // in_linesize[0] =
        //     encoder->in_width *
        //     av_get_bits_per_pixel(av_pix_fmt_desc_get(AV_PIX_FMT_RGB32)) / 8;

        // sws_scale(encoder->pSwsContext, (const uint8_t *const *)in_data,
        //           in_linesize, 0, encoder->in_height,
        //           encoder->sw_frame->data, encoder->sw_frame->linesize);
    } else {
        memset(encoder->sw_frame->data, 0, sizeof(encoder->sw_frame->data));
        memset(encoder->sw_frame->linesize, 0,
               sizeof(encoder->sw_frame->linesize));
        encoder->sw_frame->data[0] = (uint8_t *)rgb_pixels;
        encoder->sw_frame->linesize[0] =
            encoder->in_width *
            av_get_bits_per_pixel(
                av_pix_fmt_desc_get(encoder->sw_frame->format)) /
            8;
    }
    encoder->sw_frame->pts++;
    AVFrame *active_frame = encoder->sw_frame;

    if (encoder->hw_frame) {
        av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);
        encoder->hw_frame->pict_type = encoder->sw_frame->pict_type;
        active_frame = encoder->hw_frame;
    }

    int res = av_buffersrc_add_frame(encoder->pFilterGraphSource, active_frame);
    if (res < 0) {
        LOG_WARNING("Error submitting frame to the filter graph: %s",
                    av_err2str(res));
    }

    if (encoder->hw_frame) {
        av_hwframe_get_buffer(encoder->pCodecCtx->hw_frames_ctx,
                              encoder->hw_frame, 0);
    }
}

int video_encoder_encode_frame(video_encoder_t *encoder) {
    int res_buffer, res_encoder;

    // submit all available frames to the encoder
    while ((res_buffer = av_buffersink_get_frame(
                encoder->pFilterGraphSink, encoder->filtered_frame)) >= 0) {
        res_encoder =
            avcodec_send_frame(encoder->pCodecCtx, encoder->filtered_frame);

        // unref the frame so it may be reused
        av_frame_unref(encoder->filtered_frame);

        if (res_encoder < 0) {
            LOG_WARNING("Error sending frame for encoding: %s",
                        av_err2str(res_encoder));
        }
    }
    if (res_buffer < 0 && res_buffer != AVERROR(EAGAIN) &&
        res_buffer != AVERROR_EOF) {
        LOG_WARNING("Error getting frame from the filter graph: %d -- %s",
                    res_buffer, av_err2str(res_buffer));
        return -1;
    }

    av_packet_unref(&encoder->packet);
    av_init_packet(&encoder->packet);
    res_encoder = avcodec_receive_packet(encoder->pCodecCtx, &encoder->packet);
    if (res_encoder == AVERROR(EAGAIN) || res_encoder == AVERROR(EOF)) {
        return 1;
    } else if (res_encoder < 0) {
        LOG_WARNING("Error getting frame from the encoder: %s",
                    av_err2str(res_encoder));
        return -1;
    }

    encoder->encoded_frame_size = encoder->packet.size;
    encoder->encoded_frame_data = encoder->packet.data;

    return 0;
}

int video_encoder_encode(video_encoder_t *encoder, void *rgb_pixels) {
    video_encoder_filter_graph_intake(encoder, rgb_pixels);
    return video_encoder_encode_frame(encoder);
}