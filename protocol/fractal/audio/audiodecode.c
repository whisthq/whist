/*
 * Audio decoding via FFmpeg library.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "audiodecode.h"

audio_decoder_t *create_audio_decoder(int sample_rate) {
    // initialize the audio decoder

    audio_decoder_t *decoder = (audio_decoder_t *)malloc(sizeof(audio_decoder_t));
    memset(decoder, 0, sizeof(audio_decoder_t));

    // setup the AVCodec and AVFormatContext
    // avcodec_register_all is deprecated on FFmpeg 4+
    // only Linux uses FFmpeg 3.4.x because of canonical system packages
#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
#endif

    decoder->pCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!decoder->pCodec) {
        LOG_WARNING("AVCodec not found.\n");
        destroy_audio_decoder(decoder);
        return NULL;
    }
    decoder->pCodecCtx = avcodec_alloc_context3(decoder->pCodec);
    if (!decoder->pCodecCtx) {
        LOG_WARNING("Could not allocate AVCodecContext.\n");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    decoder->pCodecCtx->sample_fmt = decoder->pCodec->sample_fmts[0];
    decoder->pCodecCtx->sample_rate = sample_rate;
    decoder->pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    decoder->pCodecCtx->channels =
        av_get_channel_layout_nb_channels(decoder->pCodecCtx->channel_layout);

    if (avcodec_open2(decoder->pCodecCtx, decoder->pCodec, NULL) < 0) {
        LOG_WARNING("Could not open AVCodec.\n");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    // setup the AVFrame
    decoder->pFrame = av_frame_alloc();

    // setup the SwrContext for resampling

    decoder->pSwrContext =
        swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, sample_rate,
                           decoder->pCodecCtx->channel_layout, decoder->pCodecCtx->sample_fmt,
                           decoder->pCodecCtx->sample_rate, 0,
                           NULL);  //       might not work if not same sample size throughout

    if (!decoder->pSwrContext) {
        LOG_WARNING("Could not initialize SwrContext.\n");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    if (swr_init(decoder->pSwrContext)) {
        LOG_WARNING("Could not open SwrContext.\n");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    // everything set up, so return the decoder

    return decoder;
}

int init_av_frame(audio_decoder_t *decoder) {
    // setup the AVFrame fields

    decoder->pFrame->nb_samples = MAX_AUDIO_FRAME_SIZE;
    decoder->pFrame->format = decoder->pCodecCtx->sample_fmt;
    decoder->pFrame->channel_layout = decoder->pCodecCtx->channel_layout;

    // initialize the AVFrame buffer

    if (av_frame_get_buffer(decoder->pFrame, 0)) {
        LOG_WARNING("Could not initialize AVFrame buffer.\n");
        return -1;
    }

    return 0;
}

void audio_decoder_packet_readout(audio_decoder_t *decoder, uint8_t *data) {
    if (!decoder) return;
    // convert samples from the AVFrame and return

    // initialize
    // mprintf("reading out packet\n");

    uint8_t **data_out = &data;
    int len = decoder->pFrame->nb_samples;
    // mprintf("DECODED FRAME LEN: %d\n", len);
    // convert

    /*
    mprintf("swrcontext: %p || frame: %p || frame_data: %p\n",
            decoder->pSwrContext, decoder->pFrame,
            decoder->pFrame->extended_data);
  */

    if (swr_convert(decoder->pSwrContext, data_out, len,
                    (const uint8_t **)decoder->pFrame->extended_data, len) < 0) {
        LOG_WARNING("Could not convert samples to output format.\n");
    }
    // mprintf("finished reading out packet\n");
}

int audio_decoder_get_frame_data_size(audio_decoder_t *decoder) {
    return av_get_bytes_per_sample(decoder->pCodecCtx->sample_fmt) * decoder->pFrame->nb_samples *
           av_get_channel_layout_nb_channels(decoder->pFrame->channel_layout);
}

int audio_decoder_decode_packet(audio_decoder_t *decoder, AVPacket *encoded_packet) {
    if (!decoder) {
        return -1;
    }
    // decode a single packet of encoded data
    // mprintf("decoding packet!\n");
    // initialize output frame

    // if (init_av_frame(decoder)) {
    //     LOG_WARNING("Could not initialize output AVFrame for decoding.\n");
    //     return -1;
    // }

    // send packet for decoding

    int res = avcodec_send_packet(decoder->pCodecCtx, encoded_packet);
    if (res < 0) {
        LOG_WARNING("Could not send AVPacket for decoding: error '%s'.\n", av_err2str(res));
        return -1;
    }

    // get decoded frame

    res = avcodec_receive_frame(decoder->pCodecCtx, decoder->pFrame);
    if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
        // decoder needs more data or there's nothing left
        LOG_INFO("packet wants more things\n");
        return 1;
    } else if (res < 0) {
        // real error
        LOG_ERROR("Could not decode frame: error '%s'.\n", av_err2str(res));
        return -1;
    } else {
        // printf("Succeeded to decode frame of size:%5d\n",
        // decoder->pFrame->nb_samples);
        // mprintf("finished decoding packet!\n");
        return 0;
    }
}

void destroy_audio_decoder(audio_decoder_t *decoder) {
    if (decoder == NULL) {
        LOG_WARNING("Cannot destroy null audio decoder.\n");
        return;
    }
    LOG_INFO("destroying audio decoder!\n");

    // free the ffmpeg context
    avcodec_free_context(&decoder->pCodecCtx);

    // free the frame
    av_frame_free(&decoder->pFrame);

    // free swr
    swr_free(&decoder->pSwrContext);

    // free the buffer and decoder
    free(decoder);
}
