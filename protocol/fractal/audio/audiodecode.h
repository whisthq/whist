#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audiodecode.h
 * @brief This file contains the code to decode AAC-encoded audio using FFmpeg.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include "../core/fractal.h"

/*
============================
Defines
============================
*/

#define MAX_AUDIO_FRAME_SIZE 192000

/*
============================
Custom Types
============================
*/

typedef struct audio_decoder_t {
    AVCodec* pCodec;
    AVCodecContext* pCodecCtx;
    AVFrame* pFrame;
    SwrContext* pSwrContext;
    uint8_t* out_buffer;
} audio_decoder_t;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize a FFmpeg AAC audio decoder for a
 *                                 specific sample rate
 *
 * @param sample_rate              The sample rate, in Hertz, of the audio to
 *                                 decode
 *
 * @returns                        The initialized FFmpeg AAC audio decoder
 */
audio_decoder_t* create_audio_decoder(int sample_rate);

/**
 * @brief                          Initialize an AVFrame to receive a decoded
 *                                 audio frame
 *
 * @param decoder                  The audio decoder to associate the AVFrame
 *                                 with
 *
 * @returns                        0 if success, else -1
 */
int init_av_frame(audio_decoder_t* decoder);

/**
 * @brief                          Retrieve the size of an audio frame
 *
 * @param decoder                  The audio decoder associated with the audio
 *                                 frame
 *
 * @returns                        The size of the audio frame, in bytes
 */
int audio_decoder_get_frame_data_size(audio_decoder_t* decoder);

/**
 * @brief                          Read a decoded audio packet from the decoder
 *                                 into a data buffer
 *
 * @param decoder                  The audio decoder that decoded the audio
 *                                 packet
 * @param data                     Data buffer to receive the decoded audio data
 */
void audio_decoder_packet_readout(audio_decoder_t* decoder, uint8_t* data);

/**
 * @brief                          Decode an AAC encoded audio packet
 *
 * @param decoder                  The audio decoder used to decode the AAC
 *                                 packet
 * @param encoded_packet           The AAC encoded audio packet to decode
 *
 * @returns                        0 if success, else -1
 */

int audio_decoder_decode_packet(audio_decoder_t* decoder, AVPacket* encoded_packet);

/**
 * @brief                          Destroy a FFmpeg AAC audio decoder, and free
 *                                 its memory
 *
 * @param decoder                  The audio decoder to destroy
 */
void destroy_audio_decoder(audio_decoder_t* decoder);

#endif  // DECODE_H
