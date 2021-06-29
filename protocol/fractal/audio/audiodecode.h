#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audiodecode.h
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

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <fractal/core/fractal.h>
#include <fractal/utils/avpacket_buffer.h>

/*
============================
Defines
============================
*/

#define MAX_AUDIO_FRAME_SIZE 192000
#define MAX_ENCODED_AUDIO_PACKETS 3

/*
============================
Custom Types
============================
*/

/**
 * @brief       Struct for handling decoding and resampling of audio. The FFmpeg codec and context
 *              handle decoding packets into frame, and then swr_context resamples the data to the
 *              system output format in out_buffer.
 */
typedef struct AudioDecoder {
    const AVCodec* codec;
    AVCodecContext* context;
    AVFrame* frame;
    SwrContext* swr_context;
    AVPacket packets[MAX_ENCODED_AUDIO_PACKETS];
    uint8_t* out_buffer;
} AudioDecoder;

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
AudioDecoder* create_audio_decoder(int sample_rate);

/**
 * @brief                          Initialize an AVFrame to receive a decoded
 *                                 audio frame
 *
 * @param decoder                  The audio decoder to associate the AVFrame
 *                                 with
 *
 * @returns                        0 if success, else -1
 */
int init_av_frame(AudioDecoder* decoder);

/**
 * @brief                          Retrieve the size of an audio frame
 *
 * @param decoder                  The audio decoder associated with the audio
 *                                 frame
 *
 * @returns                        The size of the audio frame, in bytes
 */
int audio_decoder_get_frame_data_size(AudioDecoder* decoder);

/**
 * @brief                          Read a decoded audio packet from the decoder
 *                                 into a data buffer
 *
 * @param decoder                  The audio decoder that decoded the audio
 *                                 packet
 *
 * @param data                     Data buffer to receive the decoded audio data
 */
void audio_decoder_packet_readout(AudioDecoder* decoder, uint8_t* data);

/**
 * @brief                          Decode an AAC encoded audio packet
 *
 * @param decoder                  The audio decoder used to decode the AAC
 *                                 packet
 *
 * @param encoded_packet           The AAC encoded audio packet to decode
 *
 * @returns                        0 if success, else -1
 */

int audio_decoder_decode_packet(AudioDecoder* decoder, AVPacket* encoded_packet);

/**
 * @brief                          Destroy a FFmpeg AAC audio decoder, and free
 *                                 its memory
 *
 * @param decoder                  The audio decoder to destroy
 */
void destroy_audio_decoder(AudioDecoder* decoder);

/**
 * @brief                           Send the packets contained in buffer into the decoder
 *
 * @param decoder                   The decoder we are using for decoding
 *
 * @param buffer                    The buffer containing the encoded packets
 *
 * @param buffer_size               The size of the buffer containing the frame to decode
 *
 * @returns                         0 on success, -1 on failure
 */
int audio_decoder_send_packets(AudioDecoder* decoder, void* buffer, int buffer_size);

/**
 * @brief                           Get the next decoded frame from the decoder, which will be
 * placed in decoder->sw_frame.
 *
 * @param decoder                   The decoder we are using for decoding
 *
 * @returns                         0 on success (can call again), 1 on EAGAIN (send more input
 *                                  before calling again), -1 on failure
 */
int audio_decoder_get_frame(AudioDecoder* decoder);
#endif  // DECODE_H
