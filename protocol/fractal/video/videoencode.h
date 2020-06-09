#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file videoencode.h
 * @brief This file contains the code to create and destroy Encoders and use
 *        them to encode captured screens.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

#define MAX_ENCODER_PACKETS 20

typedef struct video_encoder_t {
    AVCodec* pCodec;
    AVCodecContext* pCodecCtx;
    AVFilterGraph* pFilterGraph;
    AVFilterContext* pFilterGraphSource;
    AVFilterContext* pFilterGraphSink;
    AVBufferRef* hw_device_ctx;

    int num_packets;
    AVPacket packets[MAX_ENCODER_PACKETS];

    int in_width, in_height;
    int out_width, out_height;
    int gop_size;
    void* sw_frame_buffer;
    void* encoded_frame_data;  /// <Pointer to the encoded data
    int encoded_frame_size;    /// <size of encoded frame in bytes

    AVFrame* hw_frame;
    AVFrame* sw_frame;
    AVFrame* filtered_frame;

    EncodeType type;
} video_encoder_t;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Will create a new encoder
 *
 * @param in_width                 Width of the frames that the encoder must
 *                                 intake
 * @param in_height                Height of the frames that the encoder must
 *                                 intake
 * @param out_width                Width of the frames that the encoder must
 *                                 output
 * @param out_height               Height of the frames that the encoder must
 *                                 output
 * @param bitrate                  The number of bits per second that this
 *                                 encoder will encode to
 *
 * @returns                        The newly created encoder
 */
video_encoder_t* create_video_encoder(int in_width, int in_height,
                                      int out_width, int out_height,
                                      int bitrate);

/**
 * @brief                          Put the input data into a software frame, and
 *                                 upload to a hardware frame if applicable.
 *
 * @param rgb_pixels               The frame to be in encoded
 * @param pitch                    The number of bytes per line
 *
 * @returns                        0 on success, else -1
 */
int video_encoder_frame_intake(video_encoder_t* encoder, void* rgb_pixels,
                               int pitch);

/**
 * @brief                          Encode the frame in `encoder->sw_frame` or
 *                                 `encoder->hw_frame`. The encoded packet can
 *                                 be accessed via encoded_frame_size and
 *                                 encoded_frame_data
 *
 * @returns                        0 on success, else -1
 */
int video_encoder_encode(video_encoder_t* encoder);

void video_encoder_write_buffer(video_encoder_t* encoder, int* buf);

/**
 * @brief                          Set the next frame to be an i-frame
 *
 * @param encoder                  Encoder to be updated
 */
void video_encoder_set_iframe(video_encoder_t* encoder);

/**
 * @brief                          Allow the next frame to be either an i-frame
 *                                 or not an i-frame
 *
 * @param encoder                  Encoder to be updated
 */
void video_encoder_unset_iframe(video_encoder_t* encoder);

/**
 * @brief                          Destroy encoder
 *
 * @param encoder                  Encoder to be destroyed
 */
void destroy_video_encoder(video_encoder_t* encoder);

#endif  // ENCODE_H
