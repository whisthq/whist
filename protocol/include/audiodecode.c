#include "audiodecode.h"  // header file for this file

#define MAX_AUDIO_FRAME_SIZE 192000  // 1 second of 48khz 32bit audio

audio_decoder_t *create_audio_decoder(int sample_rate) {
    // initialize the audio decoder

    audio_decoder_t *decoder =
        (audio_decoder_t *)malloc(sizeof(audio_decoder_t));
    memset(decoder, 0, sizeof(audio_decoder_t));

    // setup the AVCodec and AVFormatContext

    decoder->pCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!decoder->pCodec) {
        fprintf(stderr, "AVCodec not found.\n");
        return NULL;
    }
    decoder->pCodecCtx = avcodec_alloc_context3(decoder->pCodec);
    if (!decoder->pCodecCtx) {
        fprintf(stderr, "Could not allocate AVCodecContext.\n");
        return NULL;
    }
    decoder->pFormatCtx = avformat_alloc_context();
    if (!decoder->pFormatCtx) {
        fprintf(stderr, "Could not allocate AVFormatContext.\n");
        return NULL;
    }

    decoder->pCodecCtx->sample_fmt = decoder->pCodec->sample_fmts[0];
    decoder->pCodecCtx->sample_rate = sample_rate;
    decoder->pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    decoder->pCodecCtx->channels =
        av_get_channel_layout_nb_channels(decoder->pCodecCtx->channel_layout);

    if (avcodec_open2(decoder->pCodecCtx, decoder->pCodec, NULL) < 0) {
        printf("Could not open AVCodec.\n");
        return NULL;
    }

    // setup the AVFrame
    decoder->pFrame = av_frame_alloc();

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
        fprintf(stderr, "Could not initialize AVFrame buffer.\n");
        return -1;
    }

    return 0;
}

void audio_decoder_packet_readout(audio_decoder_t *decoder, uint8_t *data,
                                  int len) {
    // convert samples from the AVFrame and return

    // initialize

    uint8_t **data_out = &data;

    // convert

    if (swr_convert(decoder->pSwrContext, data_out, len,
                    (const uint8_t **) decoder->pFrame->extended_data, len) < 0) {
        fprintf(stderr, "Could not convert samples to output format.\n");
    }
}

int audio_decoder_decode_packet(audio_decoder_t *decoder,
                                AVPacket *encoded_packet) {
    // decode a single packed of encoded data

    // initialize output frame

    if (init_av_frame(decoder)) {
        fprintf(stderr, "Could not initialize output AVFrame for decoding.\n");
        return -1;
    }

    // printf("input packet
    // details:\n\tpts:\t%d\n\tsize:\t%d\n\tduration:\t%d\n",
    //       encoded_packet->pts, encoded_packet->size,
    //       encoded_packet->duration);
    // send packet for decoding

    int res = avcodec_send_packet(decoder->pCodecCtx, encoded_packet);
    if (res < 0) {
        fprintf(stderr, "Could not send AVPacket for decoding: error '%s'.\n",
                av_err2str(res));
        return -1;
    }

    // get decoded frame

    res = avcodec_receive_frame(decoder->pCodecCtx, decoder->pFrame);
    if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
        // decoder needs more data or there's nothing left
        return 1;
    } else if (res < 0) {
        // real error
        fprintf(stderr, "Could not decode frame: error '%s'.\n",
                av_err2str(res));
        return -1;
    } else {
        // printf("Succeeded to decode frame of size:%5d\n",
        // decoder->pFrame->nb_samples);
        return 0;
    }
}

void destroy_audio_decoder(audio_decoder_t *decoder) {
    if (decoder == NULL) {
        printf("Cannot destroy decoder decoder.\n");
        return;
    }

    // free the ffmpeg contextes
    avcodec_close(decoder->pCodecCtx);

    // free the decoder context and frame
    av_free(decoder->pCodecCtx);
    av_free(decoder->pFrame);
    av_frame_free(&decoder->pFrame);

    // free the buffer and decoder
    free(decoder);
    return;
}
