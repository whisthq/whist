/*
 * This file contains the implementation of the FFmpeg encoder functions
 * that encode the audio frames.

 Protocol version: 1.0
 Last modification: 12/22/2019

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdio.h>
#include <stdlib.h>

#include "audioencode.h" // header file for this file

audio_filter *create_audio_filter(audio_capture_device *device, audio_encoder_t *encoder)
{
    audio_filter *filter = (audio_filter *) malloc(sizeof(audio_filter));
    memset(filter, 0, sizeof(audio_filter));

    char args[512];
    int ret = 0;
    AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    AVRational time_base = device->fmt_ctx->streams[device->audio_stream_index]->time_base;
    printf("Audio time base is %d by %d\n", time_base.num, time_base.den);
    filter->filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter->filter_graph) {
        ret = AVERROR(ENOMEM);
        return -1;
    }
    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    if (!device->dec_ctx->channel_layout)
        device->dec_ctx->channel_layout = av_get_default_channel_layout(device->dec_ctx->channels);
    snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
             time_base.num, time_base.den, device->dec_ctx->sample_rate,
             av_get_sample_fmt_name(device->dec_ctx->sample_fmt), device->dec_ctx->channel_layout);
    ret = avfilter_graph_create_filter(&filter->buffersrc_ctx, abuffersrc, "in",
                                       args, NULL, filter->filter_graph);
    if (ret < 0) {
        printf("Cannot create audio buffer source\n");
        return -1;
    }
    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&filter->buffersink_ctx, abuffersink, "out",
                                       NULL, NULL, filter->filter_graph);
    if (ret < 0) {
        printf("Cannot create audio buffer sink\n");
        return -1;
    }
    ret = av_opt_set_bin(filter->buffersink_ctx, "sample_fmts", (uint8_t*)&encoder->context->sample_fmt, sizeof(device->dec_ctx->sample_fmt),
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output sample format\n");
        return -1;
    }
    ret = av_opt_set_bin(filter->buffersink_ctx, "channel_layouts", (uint8_t*)&encoder->context->channel_layout, sizeof(device->dec_ctx->channel_layout),
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output channel layout\n");
        return -1;
    }
    ret = av_opt_set_bin(filter->buffersink_ctx, "sample_rates", (uint8_t*)&encoder->context->sample_rate, sizeof(device->dec_ctx->sample_rate),
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        printf("Cannot set output sample rate\n");
        return -1;
    }
    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = filter->buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = filter->buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    char *filter_descr = "aresample=8000,aformat=sample_fmts=s16:channel_layouts=mono";
    if ((ret = avfilter_graph_parse_ptr(filter->filter_graph, filter_descr,
                                        &inputs, &outputs, NULL)) < 0)
        return -1;
    if ((ret = avfilter_graph_config(filter->filter_graph, NULL)) < 0)
        return -1;

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return filter;
}

int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* just pick the highest supported samplerate */
int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    printf("best sample rate is %d\n", best_samplerate);
    return best_samplerate;
}

/* select layout with the highest channel count */
int select_channel_layout(const AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;

    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;

    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);

        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    printf("best channel layout %d\n", best_ch_layout);
    return best_ch_layout;
}

audio_encoder_t *create_audio_encoder(int bit_rate) {
    audio_encoder_t *encoder = (audio_encoder_t *) malloc(sizeof(audio_encoder_t));
    memset(encoder, 0, sizeof(audio_encoder_t));
    
    encoder->codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!encoder->codec) {
        printf("Codec not found\n");
        exit(1);
    }

    encoder->context = avcodec_alloc_context3(encoder->codec);
    if (!encoder->context) {
        printf("Could not allocate audio codec context\n");
        exit(1);
    }

    /* put sample parameters */

    encoder->context->sample_fmt = encoder->codec->sample_fmts[0];
    if (!check_sample_fmt(encoder->codec, encoder->context->sample_fmt)) {
        printf("Encoder does not support sample format %s",
                av_get_sample_fmt_name(encoder->context->sample_fmt));
        exit(1);
    }

    /* select other audio parameters supported by the encoder */
    encoder->context->bit_rate       = bit_rate;
    encoder->context->sample_rate    = 44100;
    encoder->context->channel_layout = 3;
    encoder->context->channels       = av_get_channel_layout_nb_channels(encoder->context->channel_layout);
    encoder->context->time_base.num = 1;
    encoder->context->time_base.den = 30;
    // encoder->context->gop_size = 1;

    /* open it */
    if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
        printf("Could not open codec\n");
        exit(1);
    }

    encoder->input_frame = av_frame_alloc();
    if (!encoder->input_frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    encoder->input_frame->nb_samples     = encoder->context->frame_size;
    encoder->input_frame->format         = encoder->context->sample_fmt;
    encoder->input_frame->channel_layout = encoder->context->channel_layout;

    /* allocate the data buffers */
    int ret;
    ret = av_frame_get_buffer(encoder->input_frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }

    return encoder;
}

void audio_encode_and_send(audio_capture_device *device, audio_encoder_t *encoder, audio_filter *filter, SocketContext context) {
    int got_frame, got_output, sent_size, slen = sizeof(context.addr);

    encoder->output_frame = av_frame_alloc();
    av_init_packet(&encoder->packet);

    if(av_read_frame(device->fmt_ctx, &encoder->packet) < 0){
      printf("Error reading frame\n");
      exit(1);
    }

    if (encoder->packet.stream_index == device->audio_stream_index) {
        avcodec_decode_audio4(device->dec_ctx, encoder->input_frame, &got_frame, &encoder->packet);
        encoder->input_frame->pts = av_frame_get_best_effort_timestamp(encoder->input_frame);
        if (got_frame) {
            av_packet_unref(&encoder->packet);
            av_init_packet(&encoder->packet);
            /* push the audio data from decoded frame into the filtergraph */
            av_buffersrc_add_frame_flags(filter->buffersrc_ctx, encoder->input_frame, 0);
            /* pull filtered audio from the filtergraph */
            while (1) {
                int ret = av_buffersink_get_frame(filter->buffersink_ctx, encoder->output_frame);
                if (ret < 0) {
                    // if there are no more frames or end of frames that is normal
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                      ret = 0;
                    av_frame_free(&encoder->output_frame);
                    break;
                }
                avcodec_encode_audio2(encoder->context, &encoder->packet, encoder->output_frame, &got_output);
                if((&encoder->packet.size != 0) && (got_output)) {
                    if ((sent_size = sendto(context.s, encoder->packet.data, encoder->packet.size, 0, (struct sockaddr*)(&context.addr), slen)) < 0) {
    			        // error statement if something went wrong
    			    	printf("Socket could not send packet w/ error %d\n", WSAGetLastError());
    			    }
                }
            }
            av_packet_unref(&encoder->packet);
        }
    } else {
        /* discard non-wanted packets */
        av_frame_free(&encoder->output_frame);
    }
}