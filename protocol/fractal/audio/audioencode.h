#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audioencode.h
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

/*
============================
Includes
============================
*/

#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

typedef struct audio_encoder_t {
    AVCodec* pCodec;
    AVCodecContext* pCodecCtx;
    AVFrame* pFrame;
    AVAudioFifo* pFifo;
    AVPacket packet;
    SwrContext* pSwrContext;
    int frame_count;
    int encoded_frame_size;
    void* encoded_frame_data;
} audio_encoder_t;

typedef struct encoded_audio_t {
    int pts;
    int size;
    uint8_t data[10];
} encoded_audio_t;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          This will initialize the FFmpeg AAC audio
 *                                 encoder, and set the proper audio parameters
 *                                 for receiving from the server
 *
 * @param bit_rate                 The amount of bits/seconds that the audio
 *                                 will be encoded to (higher means higher
 *                                 quality encoding, and more bandwidth usage)
 * @param sample_rate              The sample rate, in Hertz, of the audio to
 *                                 encode
 *
 * @returns                        The initialized FFmpeg audio encoder struct
 */
audio_encoder_t* create_audio_encoder(int bit_rate, int sample_rate);

/**
 * @brief                          Feeds raw audio data to the FIFO queue, which
 *                                 is pulled from by the encoder to encode AAC
 *                                 frames
 *
 * @param encoder                  The audio encoder struct used to AAC encode a
 *                                 frame
 * @param data                     Buffer of the audio data to intake in the
 *                                 encoder FIFO queue to encode
 * @param len                      Length of the buffer of data to intake
 */
void audio_encoder_fifo_intake(audio_encoder_t* encoder, uint8_t* data, int len);

/**
 * @brief                          Encodes an AVFrame of audio to AAC format
 *
 * @param encoder                  The audio encoder struct used to AAC encode a
 *                                 frame
 *
 * @returns                        0 if success, else -1
 */
int audio_encoder_encode_frame(audio_encoder_t* encoder);

/**
 * @brief                          Destroys and frees the FFmpeg audio encoder
 *
 * @param encoder                  The audio encoder struct to destroy and free
 *                                 the memory of
 */
void destroy_audio_encoder(audio_encoder_t* encoder);

#endif  // ENCODE_H
