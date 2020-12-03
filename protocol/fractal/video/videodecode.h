#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file videodecode.h
 * @brief This file contains the code to create a video decoder and use that
 *        decoder to decode frames.
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
Includes
============================
*/

typedef enum DecodeType {
    DECODE_TYPE_NONE = 0,
    DECODE_TYPE_SOFTWARE = 1,
    DECODE_TYPE_HARDWARE = 2,
    DECODE_TYPE_QSV = 3,
    DECODE_TYPE_HARDWARE_OLDER = 4,
} DecodeType;

typedef struct VideoDecoder {
    int width;
    int height;
    bool can_use_hardware;
    AVCodec* codec;
    AVCodecContext* context;
    AVFrame* sw_frame;
    AVFrame* hw_frame;
    AVBufferRef* ref;
    enum AVPixelFormat match_fmt;
    DecodeType type;
    CodecType codec_type;
    enum AVHWDeviceType device_type;
} VideoDecoder;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          InitializeThis will initialize the FFmpeg AAC
 *                                 video decoder, and set the proper video
 *                                 parameters for receiving from the server
 *
 * @param width                    Width of the frames to decode
 * @param height                   Height of the frames to decode
 * @param use_hardware             Toggle whether to try to decode in hardware
 * @param codec_type               Which codec type (h264 or h265) to use
 *
 * @returns                        The FFmpeg video decoder struct
 */
VideoDecoder* create_video_decoder(int width, int height, bool use_hardware, CodecType codec_type);

/**
 * @brief                          Destroys an initialized FFmpeg video decoder
 *                                 and frees its memory
 *
 * @param decoder                  The FFmpeg video decoder to destroy
 */
void destroy_video_decoder(VideoDecoder* decoder);

/**
 * @brief                          Decode a compressed video frame using the
 *                                 FFmpeg decoder
 *
 * @param decoder                  The initialized FFmepg decoder used to decode
 * @param buffer                   The buffer containing the frame to decode
 * @param buffer_size              The size of the buffer containing the frame
 *                                 to decode
 *
 * @returns                        True if it decoded successfully, else False
 */
bool video_decoder_decode(VideoDecoder* decoder, void* buffer, int buffer_size);

#endif  // VIDEO_DECODE_H
