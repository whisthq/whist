/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audioencode.c
 * @brief This file contains the code to encode AAC-encoded audio using FFmpeg.
============================
Usage
============================

Audio is encoded to AAC via FFmpeg using a FIFO queue. In order for FFmpeg to
be able to encode an audio frame, it needs to be have a certain duration of
data. This is frequently more than a single packet, which is why we have a FIFO
queue. You can initialize the AAC encoder via create_audio_encoder. You then
receive packets into the FIFO queue, which is a data buffer, via
audio_encoder_fifo_intake. You can then encode via audio_encoder_encode.
*/

#include "audioencode.h"

int audio_encoder_receive_packet(AudioEncoder* encoder, AVPacket* packet);

/*
============================
Public Function Implementations
============================
*/

AudioEncoder* create_audio_encoder(int bit_rate, int sample_rate) {
    /*
        Initialize the FFmpeg AAC audio encoder, and set the proper audio parameters
        for receiving from the server

        Arguments:
            bit_rate (int): The amount of bits/seconds that the audio will be encoded
                to (higher means higher quality encoding, and more bandwidth usage)
            sample_rate (int): The sample rate, in Hertz, of the audio to encode

        Returns:
            (AudioEncoder*): The initialized FFmpeg audio encoder struct
    */

    // initialize the audio encoder
    AudioEncoder* encoder = (AudioEncoder*)safe_malloc(sizeof(AudioEncoder));
    memset(encoder, 0, sizeof(*encoder));

    // setup the AVCodec and AVFormatContext
    encoder->codec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!encoder->codec) {
        LOG_WARNING("AVCodec not found.");
        destroy_audio_encoder(encoder);
        return NULL;
    }
    encoder->context = avcodec_alloc_context3(encoder->codec);
    if (!encoder->context) {
        LOG_WARNING("Could not allocate AVCodecContext.");
        destroy_audio_encoder(encoder);
        return NULL;
    }

    // set the context's fields to agree with that of the codec (see ffmpeg documentation for info
    // on each codec)
    encoder->context->codec_id = AV_CODEC_ID_AAC;
    encoder->context->codec_type = AVMEDIA_TYPE_AUDIO;
    encoder->context->sample_fmt = encoder->codec->sample_fmts[0];
    encoder->context->sample_rate = sample_rate;
    encoder->context->channel_layout = AV_CH_LAYOUT_STEREO;
    encoder->context->channels =
        av_get_channel_layout_nb_channels(encoder->context->channel_layout);
    encoder->context->bit_rate = bit_rate;

    if (avcodec_open2(encoder->context, encoder->codec, NULL) < 0) {
        LOG_WARNING("Could not open AVCodec.");
        destroy_audio_encoder(encoder);
        return NULL;
    }

    // setup the AVFrame

    encoder->frame = av_frame_alloc();
    encoder->frame->nb_samples = encoder->context->frame_size;
    encoder->frame->format = encoder->context->sample_fmt;
    encoder->frame->channel_layout = AV_CH_LAYOUT_STEREO;

    encoder->frame_count = 0;

    // initialize the AVFrame buffer

    if (av_frame_get_buffer(encoder->frame, 0)) {
        LOG_WARNING("Could not initialize AVFrame buffer.");
        destroy_audio_encoder(encoder);
        return NULL;
    }

    // initialize the AVAudioFifo as an empty FIFO

    encoder->audio_fifo =
        av_audio_fifo_alloc(encoder->frame->format,
                            av_get_channel_layout_nb_channels(encoder->frame->channel_layout), 1);
    if (!encoder->audio_fifo) {
        LOG_WARNING("Could not allocate AVAudioFifo.");
        destroy_audio_encoder(encoder);
        return NULL;
    }

    // setup the SwrContext for resampling alsa audio into the FDK-AAC format (16-bit audio)

    encoder->swr_context = swr_alloc_set_opts(
        NULL, encoder->frame->channel_layout, encoder->frame->format, encoder->context->sample_rate,
        AV_CH_LAYOUT_STEREO,  // should get layout from alsa
        AV_SAMPLE_FMT_FLT,    // should get format from alsa, which uses SND_PCM_FORMAT_FLOAT_LE
        sample_rate,          // should use same sample rate as alsa, though this just
        0, NULL);             //       might not work if not same sample size throughout
    if (!encoder->swr_context) {
        LOG_WARNING("Could not initialize SwrContext.");
        destroy_audio_encoder(encoder);
        return NULL;
    }

    if (swr_init(encoder->swr_context)) {
        LOG_WARNING("Could not open SwrContext.");
        destroy_audio_encoder(encoder);
        return NULL;
    }

    // everything set up, so return the encoder

    return encoder;
}

void audio_encoder_fifo_intake(AudioEncoder* encoder, uint8_t* data, int len) {
    /*
        Feeds raw audio data to the FIFO queue, which is pulled from by the encoder
        to encode AAC frames

        Arguments:
            encoder (AudioEncoder*): The audio encoder struct used to AAC encode a frame
            data (uint8_t*): Buffer of the audio data to intake in the encoder FIFO
                queue to encode
            len (int): Length of the buffer of data to intake
    */

    // initialize
    uint8_t** converted_data =
        calloc(av_get_channel_layout_nb_channels(encoder->frame->channel_layout), sizeof(uint8_t*));
    if (!converted_data) {
        LOG_WARNING("Could not allocate converted samples channel pointers.");
        return;
    }

    if (av_samples_alloc(converted_data, NULL,
                         av_get_channel_layout_nb_channels(encoder->frame->channel_layout), len,
                         encoder->frame->format, 0) < 0) {
        LOG_WARNING("Could not allocate converted samples channel arrays.");
        return;
    }

    // convert
    if (swr_convert(encoder->swr_context, converted_data, len, (const uint8_t**)&data, len) < 0) {
        LOG_WARNING("Could not convert samples to intake format.");
        return;
    }

    // reallocate fifo
    if (av_audio_fifo_realloc(encoder->audio_fifo, av_audio_fifo_size(encoder->audio_fifo) + len) <
        0) {
        LOG_WARNING("Could not reallocate AVAudioFifo.");
        return;
    }

    // add
    if (av_audio_fifo_write(encoder->audio_fifo, (void**)converted_data, len) < len) {
        LOG_WARNING("Could not write all the requested data to the AVAudioFifo.");
        return;
    }

    // cleanup
    if (converted_data) {
        av_freep(&converted_data[0]);
        free(converted_data);
    }
}

int audio_encoder_encode_frame(AudioEncoder* encoder) {
    /*
        Encodes a single AVFrame of audio from the FIFO data to AAC format

        Arguments:
            encoder (AudioEncoder*): The audio encoder struct used to AAC encode a frame

        Returns:
            (int): 0 if success, else -1
    */

    // read from FIFO to AVFrame
    const int len = FFMIN(av_audio_fifo_size(encoder->audio_fifo), encoder->context->frame_size);

    if (av_audio_fifo_read(encoder->audio_fifo, (void**)encoder->frame->data, len) < len) {
        LOG_WARNING("Could not read all the requested data from the AVAudioFifo.");
        return -1;
    }

    // set frame timestamp
    encoder->frame->pts = encoder->frame_count;

    // send frame for encoding
    int res = avcodec_send_frame(encoder->context, encoder->frame);
    if (res == AVERROR_EOF) {
        // end of file
        return -1;
    } else if (res < 0) {
        // real error
        LOG_ERROR("Could not send audio AVFrame for encoding: error '%s'.", av_err2str(res));
        return -1;
    }

    // get encoded packet
    // because this always calls av_packet_unref before doing anything,
    // our previous calls to av_packet_unref are unnecessary.
    encoder->encoded_frame_size = sizeof(int);
    encoder->num_packets = 0;
    while ((res = audio_encoder_receive_packet(encoder, &encoder->packets[encoder->num_packets])) ==
           0) {
        encoder->encoded_frame_size += sizeof(int) + encoder->packets[encoder->num_packets].size;
        encoder->num_packets++;
        if (encoder->num_packets == MAX_NUM_AUDIO_PACKETS) {
            LOG_ERROR("Audio encoder encoded into too many packets: reached %d",
                      encoder->num_packets);
            return -1;
        }
    }
    if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
        // Need more data
        return 1;
    } else if (res < 0) {
        LOG_ERROR("Failed to receive packet, error: %s", av_err2str(res));
        return -1;
    }
    // set frame count to number of samples as computed by ffmpeg
    encoder->frame_count += encoder->frame->nb_samples;
    return 0;
}

void destroy_audio_encoder(AudioEncoder* encoder) {
    /*
        Destroys and frees the FFmpeg audio encoder

        Arguments:
            encoder (AudioEncoder*): The audio encoder struct to destroy and free
                the memory of
    */

    if (encoder == NULL) {
        LOG_ERROR("Cannot destroy null encoder.\n");
        return;
    }

    // free the ffmpeg contexts
    avcodec_free_context(&encoder->context);

    // free the encoder context and frame
    av_frame_free(&encoder->frame);
    LOG_INFO("av_freed frame\n");
    av_free(encoder->frame);

    // free the packets
    for (int i = 0; i < MAX_NUM_AUDIO_PACKETS; i++) {
        av_packet_unref(&encoder->packets[i]);
    }

    // free swr
    swr_free(&encoder->swr_context);
    LOG_INFO("freed swr\n");
    // free the encoder
    free(encoder);
    LOG_INFO("done destroying decoder!\n");
}

int audio_encoder_receive_packet(AudioEncoder* encoder, AVPacket* packet) {
    /*
        Wrapper around avcodec_receive_packet.

        Arguments:
            encoder (AudioEncoder*): audio encoder we want to use for encoding
            packet (AVPacket*): packet to we fill with the encoded data

        Returns:
            (int): 0 on success (can call this function again), 1 on EAGAIN (send more input before
       calling again), -1 on failure.
            */
    int res_encoder;
    res_encoder = avcodec_receive_packet(encoder->context, packet);
    if (res_encoder == AVERROR(EAGAIN)) {
        return 1;
    } else if (res_encoder < 0) {
        LOG_ERROR("Error receiving packet from encoder: %s", av_err2str(res_encoder));
    }

    return 0;
}
