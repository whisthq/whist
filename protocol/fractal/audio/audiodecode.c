/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audiodecode.c
 * @brief This file contains the code to decode AAC-encoded audio using FFmpeg.
============================
Usage
============================

Audio is decoded from AAC via FFmpeg. In order for FFmpeg to be able to decoder
an audio frame, it needs to be have a certain duration of data. This is
frequently more than a single packet, which is why we have a FIFO encoding
queue. This is abstracted away in the decoder, each packet will already have
enough data from the way the encoder encodes. You can initialize the AAC decoder
via create_audio_decoder. You then decode packets via
audio_decoder_decode_packet and convert them into readable format via
audio_decoder_packet_readout.
*/

#include "audiodecode.h"

/*
============================
Public Function Implementations
============================
*/

AudioDecoder *create_audio_decoder(int sample_rate) {
    /*
        Initialize a FFmpeg AAC audio decoder for a specific sample rate

        Arguments:
            sample_rate (int): The sample rate, in Hertz, of the audio to
                decode

        Returns:
            (AudioDecoder*): The initialized FFmpeg AAC audio decoder
    */

    // initialize the audio decoder
    AudioDecoder *decoder = (AudioDecoder *)safe_malloc(sizeof(AudioDecoder));
    memset(decoder, 0, sizeof(*decoder));

    // setup the AVCodec and AVFormatContext
    // avcodec_register_all is deprecated on FFmpeg 4+
    // only Linux uses FFmpeg 3.4.x because of canonical system packages
#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
#endif

    decoder->pCodec = avcodec_find_decoder_by_name("libfdk_aac");
    if (!decoder->pCodec) {
        LOG_WARNING("AVCodec not found.");
        destroy_audio_decoder(decoder);
        return NULL;
    }
    decoder->pCodecCtx = avcodec_alloc_context3(decoder->pCodec);
    if (!decoder->pCodecCtx) {
        LOG_WARNING("Could not allocate AVCodecContext.");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    // if we are using libfdk_aac, the codec has no sample_fmts
    // the context's sample_fmt is set during decoder initialization
    if (!decoder->pCodec->sample_fmts) {
        LOG_INFO(
            "No sample formats found in pCodec. Assuming that pCodecCtx's sample_fmt is "
            "set automatically during initialization to FDK AAC's AV_SAMPLE_FMT_S16.");
    } else {
        decoder->pCodecCtx->sample_fmt = decoder->pCodec->sample_fmts[0];
    }

    decoder->pCodecCtx->sample_rate = sample_rate;
    decoder->pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    decoder->pCodecCtx->channels =
        av_get_channel_layout_nb_channels(decoder->pCodecCtx->channel_layout);

    if (avcodec_open2(decoder->pCodecCtx, decoder->pCodec, NULL) < 0) {
        LOG_WARNING("Could not open AVCodec.");
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
        LOG_WARNING("Could not initialize SwrContext.");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    if (swr_init(decoder->pSwrContext)) {
        LOG_WARNING("Could not open SwrContext.");
        destroy_audio_decoder(decoder);
        return NULL;
    }

    // everything set up, so return the decoder

    return decoder;
}

int init_av_frame(AudioDecoder *decoder) {
    /*
        Initialize an AVFrame to receive a decoded audio frame

        Arguments:
            decoder (AudioDecoder*): The audio decoder to associate the AVFrame with

        Returns:
            (int): 0 if success, else -1
    */

    // setup the AVFrame fields
    decoder->pFrame->nb_samples = MAX_AUDIO_FRAME_SIZE;
    decoder->pFrame->format = decoder->pCodecCtx->sample_fmt;
    decoder->pFrame->channel_layout = decoder->pCodecCtx->channel_layout;

    // initialize the AVFrame buffer
    if (av_frame_get_buffer(decoder->pFrame, 0)) {
        LOG_WARNING("Could not initialize AVFrame buffer.");
        return -1;
    }

    return 0;
}

void audio_decoder_packet_readout(AudioDecoder *decoder, uint8_t *data) {
    /*
        Read a decoded audio packet from the decoder into a data buffer

        Arguments:
            decoder (AudioDecoder*): The audio decoder that decoded the audio packet
            data (uint8_t*): Data buffer to receive the decoded audio data
    */

    if (!decoder) return;
    // convert samples from the AVFrame and return

    // initialize
    uint8_t **data_out = &data;
    int len = decoder->pFrame->nb_samples;

    // convert
    if (swr_convert(decoder->pSwrContext, data_out, len,
                    (const uint8_t **)decoder->pFrame->extended_data, len) < 0) {
        LOG_WARNING("Could not convert samples to output format.");
    }
}

int audio_decoder_get_frame_data_size(AudioDecoder *decoder) {
    /*
        Retrieve the size of an audio frame

        Arguments:
            decoder (AudioDecoder*): The audio decoder associated with the audio frame

        Returns:
            (int): The size of the audio frame, in bytes
    */

    return av_get_bytes_per_sample(decoder->pCodecCtx->sample_fmt) * decoder->pFrame->nb_samples *
           av_get_channel_layout_nb_channels(decoder->pFrame->channel_layout);
}

int audio_decoder_decode_packet(AudioDecoder *decoder, AVPacket *encoded_packet) {
    /*
        Decode an AAC encoded audio packet

        Arguments:
            decoder (AudioDecoder*): The audio decoder used to decode the AAC packet
            encoded_packet (AVPacket*): The AAC encoded audio packet to decode

        Returns:
            (int): 0 if success, else -1
    */

    if (!decoder) {
        return -1;
    }

    // send packet for decoding
    int res = avcodec_send_packet(decoder->pCodecCtx, encoded_packet);
    if (res < 0) {
        LOG_WARNING("Could not send AVPacket for decoding: error '%s'.", av_err2str(res));
        return -1;
    }

    // get decoded frame
    res = avcodec_receive_frame(decoder->pCodecCtx, decoder->pFrame);
    if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
        // decoder needs more data or there's nothing left
        LOG_INFO("packet wants more things");
        if (res == AVERROR(EAGAIN)) {
          LOG_DEBUG("averror eagain");
        } else {
          LOG_DEBUG("averror eof");
        }
        return 1;
    } else if (res < 0) {
        // real error
        LOG_ERROR("Could not decode frame: error '%s'.", av_err2str(res));
        return -1;
    } else {
        return 0;
    }
}

void destroy_audio_decoder(AudioDecoder *decoder) {
    /*
        Destroy a FFmpeg AAC audio decoder, and free its memory

        Arguments:
            decoder (AudioDecoder*): The audio decoder to destroy
    */

    if (decoder == NULL) {
        LOG_WARNING("Cannot destroy null audio decoder.");
        return;
    }
    LOG_INFO("destroying audio decoder!");

    // free the ffmpeg context
    avcodec_free_context(&decoder->pCodecCtx);

    // free the frame
    av_frame_free(&decoder->pFrame);

    // free swr
    swr_free(&decoder->pSwrContext);

    // free the buffer and decoder
    free(decoder);
}
