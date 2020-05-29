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
 * @brief                          Initialize a FFmpeg AAC audio decoder for a specific sample rate
 * 
 * @param sample_rate              The sample rate, in Hertz, of the audio to decode
 * 
 * @returns                        The initialized FFmpeg AAC audio decoder
 */
audio_decoder_t* create_audio_decoder(int sample_rate);







/**
 * @brief                          Initialize a FFmpeg AAC audio decoder for a specific sample rate
 * 
 * @param decoder                  The audio decoder The sample rate, in Hertz, of the audio to decode
 * 
 * @returns                        The initialized FFmpeg AAC audio decoder
 */

int init_av_frame(audio_decoder_t* decoder);

/**
 * @brief                          Initialize a FFmpeg AAC audio decoder for a specific sample rate
 * 
 * @param decoder                  The audio decoder The sample rate, in Hertz, of the audio to decode
 * 
 * @returns                        The initialized FFmpeg AAC audio decoder
 */

int audio_decoder_get_frame_data_size(audio_decoder_t* decoder);


/**
 * @brief                          Initialize a FFmpeg AAC audio decoder for a specific sample rate
 * 
 * @param decoder                  The audio decoder The sample rate, in Hertz, of the audio to decode
 * 
 * @returns                        The initialized FFmpeg AAC audio decoder
 */

void audio_decoder_packet_readout(audio_decoder_t* decoder, uint8_t* data);

/**
 * @brief                          Initialize a FFmpeg AAC audio decoder for a specific sample rate
 * 
 * @param decoder                  The audio decoder The sample rate, in Hertz, of the audio to decode
 * 
 * @returns                        The initialized FFmpeg AAC audio decoder
 */

int audio_decoder_decode_packet(audio_decoder_t* decoder, AVPacket* encoded_packet);

/**
 * @brief                          Initialize a FFmpeg AAC audio decoder for a specific sample rate
 * 
 * @param decoder                  The audio decoder The sample rate, in Hertz, of the audio to decode
 * 
 * @returns                        The initialized FFmpeg AAC audio decoder
 */

void destroy_audio_decoder(audio_decoder_t* decoder);





#endif  // DECODE_H
