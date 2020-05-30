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

typedef struct encoder_t {
    AVCodec* pCodec;
    AVCodecContext* pCodecCtx;
    AVFilterGraph* pFilterGraph;
    AVFilterContext* pFilterGraphSource;
    AVFilterContext* pFilterGraphSink;
    AVBufferRef* hw_device_ctx;
    AVPacket packet;

    int in_width, in_height;
    int out_width, out_height;
    int gop_size;
    void* sw_frame_buffer;
    void* encoded_frame_data; /// <Pointer to the encoded data
    int encoded_frame_size;   /// <size of encoded frame in bytes

    AVFrame* hw_frame;
    AVFrame* sw_frame;
    AVFrame* filtered_frame;

    EncodeType type;
} encoder_t;

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
encoder_t* create_video_encoder(int in_width, int in_height, int out_width,
                                int out_height, int bitrate);


/**
 * @brief                          Encode the given frame. The frame can be
 *                                 accessed via encoded_frame_size and
 *                                 encoded_frame_data
 *
 * @param rgb_pixels               The frame to be in encoded
 * @param pitch                    The number of bytes per line
 */
int video_encoder_encode(encoder_t* encoder, void* rgb_pixels, int pitch);

/**
 * @brief                          Set the next frame to be an i-frame
 *
 * @param encoder                  Encoder to be updated
 */
void video_encoder_set_iframe(encoder_t* encoder);

/**
 * @brief                          Allow the next frame to be either an i-frame
 *                                 or not an i-frame
 *
 * @param encoder                  Encoder to be updated
 */
void video_encoder_unset_iframe(encoder_t* encoder);

/**
 * @brief                          Destroy encoder
 *
 * @param encoder                  Encoder to be destroyed
 */
void destroy_video_encoder(encoder_t* encoder);

#endif  // ENCODE_H
