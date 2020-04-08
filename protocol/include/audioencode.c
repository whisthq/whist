#include "audioencode.h"  // header file for this file

audio_encoder_t* create_audio_encoder(int bit_rate, int sample_rate) {
    // initialize the audio encoder

    audio_encoder_t* encoder =
        (audio_encoder_t*)malloc(sizeof(audio_encoder_t));
    memset(encoder, 0, sizeof(audio_encoder_t));

    // setup the AVCodec and AVCodecCtx

    encoder->pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!encoder->pCodec) {
        mprintf("AVCodec not found.\n");
        return NULL;
    }
    encoder->pCodecCtx = avcodec_alloc_context3(encoder->pCodec);
    if (!encoder->pCodecCtx) {
        mprintf("Could not allocate AVCodecContext.\n");
        return NULL;
    }

    encoder->pCodecCtx->codec_id = AV_CODEC_ID_AAC;
    encoder->pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    encoder->pCodecCtx->sample_fmt = encoder->pCodec->sample_fmts[0];
    encoder->pCodecCtx->sample_rate = sample_rate;
    encoder->pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    encoder->pCodecCtx->channels =
        av_get_channel_layout_nb_channels(encoder->pCodecCtx->channel_layout);
    encoder->pCodecCtx->bit_rate = bit_rate;

    if (avcodec_open2(encoder->pCodecCtx, encoder->pCodec, NULL) < 0) {
        mprintf("Could not open AVCodec.\n");
        return NULL;
    }

    // setup the AVFrame

    encoder->pFrame = av_frame_alloc();
    encoder->pFrame->nb_samples = encoder->pCodecCtx->frame_size;
    encoder->pFrame->format = encoder->pCodecCtx->sample_fmt;
    encoder->pFrame->channel_layout = AV_CH_LAYOUT_STEREO;

    encoder->frame_count = 0;

    // initialize the AVFrame buffer

    if (av_frame_get_buffer(encoder->pFrame, 0)) {
        mprintf("Could not initialize AVFrame buffer.\n");
        return NULL;
    }

    // initialize the AVAudioFifo as an empty FIFO

    encoder->pFifo = av_audio_fifo_alloc(
        encoder->pFrame->format,
        av_get_channel_layout_nb_channels(encoder->pFrame->channel_layout), 1);
    if (!encoder->pFifo) {
        mprintf("Could not allocate AVAudioFifo.\n");
        return NULL;
    }

    // setup the SwrContext for resampling

    encoder->pSwrContext = swr_alloc_set_opts(
        NULL, encoder->pFrame->channel_layout, encoder->pFrame->format,
        encoder->pCodecCtx->sample_rate,
        AV_CH_LAYOUT_STEREO,  // should get layout from WASAPI
        AV_SAMPLE_FMT_FLT,    // should get format from WASAPI
        sample_rate,  // should use same sample rate as WASAPI, though this just
        0, NULL);     //       might not work if not same sample size throughout
    if (!encoder->pSwrContext) {
        mprintf("Could not initialize SwrContext.\n");
        return NULL;
    }

    if (swr_init(encoder->pSwrContext)) {
        mprintf("Could not open SwrContext.\n");
        return NULL;
    }

    // everything set up, so return the encoder

    return encoder;
}

void audio_encoder_fifo_intake(audio_encoder_t* encoder, uint8_t* data,
                               int len) {
    // convert and add samples to the FIFO

    // initialize

    uint8_t** converted_data = calloc(
        av_get_channel_layout_nb_channels(encoder->pFrame->channel_layout),
        sizeof(uint8_t*));
    if (!converted_data) {
        mprintf("Could not allocate converted samples channel pointers.\n");
        return;
    }

    if (av_samples_alloc(
            converted_data, NULL,
            av_get_channel_layout_nb_channels(encoder->pFrame->channel_layout),
            len, encoder->pFrame->format, 0) < 0) {
        mprintf("Could not allocate converted samples channel arrays.\n");
        return;
    }

    // convert

    if (swr_convert(encoder->pSwrContext, converted_data, len, &data, len) <
        0) {
        mprintf("Could not convert samples to intake format.\n");
        return;
    }

    // reallocate fifo

    if (av_audio_fifo_realloc(encoder->pFifo,
                              av_audio_fifo_size(encoder->pFifo) + len) < 0) {
        mprintf("Could not reallocate AVAudioFifo.\n");
        return;
    }

    // add

    if (av_audio_fifo_write(encoder->pFifo, (void**)converted_data, len) <
        len) {
        mprintf("Could not write all the requested data to the AVAudioFifo.\n");
        return;
    }

    // cleanup

    if (converted_data) {
        av_freep(&converted_data[0]);
        free(converted_data);
    }
}

int audio_encoder_encode_frame(audio_encoder_t* encoder) {
    // encode a single frame from the FIFO data

    // read from FIFO to AVFrame

    const int len = FFMIN(av_audio_fifo_size(encoder->pFifo),
                          encoder->pCodecCtx->frame_size);

    if (av_audio_fifo_read(encoder->pFifo, (void**)encoder->pFrame->data, len) <
        len) {
        mprintf(
            stderr,
            "Could not read all the requested data from the AVAudioFifo.\n");
        return -1;
    }

    // initialize packet

    av_init_packet(&encoder->packet);

    // set frame timestamp
    encoder->pFrame->pts = encoder->frame_count;

    // send frame for encoding

    int res = avcodec_send_frame(encoder->pCodecCtx, encoder->pFrame);
    if (res == AVERROR_EOF) {
        // end of file
        av_packet_unref(&encoder->packet);
        return -1;
    } else if (res < 0) {
        // real error
        mprintf("Could not send AVFrame for encoding: error '%s'.\n",
                av_err2str(res));
        return -1;
    }

    // get encoded packet

    res = avcodec_receive_packet(encoder->pCodecCtx, &encoder->packet);
    if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
        // encoder needs more data or there's nothing left
        av_packet_unref(&encoder->packet);
        return 1;
    } else if (res < 0) {
        // real error
        mprintf("Could not encode frame: error '%s'.\n", av_err2str(res));
        return -1;
    } else {
        // we did it!
        encoder->frame_count += encoder->pFrame->nb_samples;
        return 0;
    }
}

void destroy_audio_encoder(audio_encoder_t* encoder) {
    if (encoder == NULL) {
        mprintf("Cannot destroy null encoder.\n");
        return;
    }

    // free the ffmpeg contexts
    avcodec_free_context(&encoder->pCodecCtx);

    // free the encoder context and frame
    av_frame_free(&encoder->pFrame);
    av_free(encoder->pFrame);

    // free swr
    swr_free(&encoder->pSwrContext);

    // free the buffer and decoder
    free(encoder);
}
