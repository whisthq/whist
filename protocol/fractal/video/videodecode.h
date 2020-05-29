#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file videodecode.h
 * @brief This file contains the code to create a decoder and use that decoder to decode frames.
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

// define decoder struct to pass as a type
typedef struct video_decoder_t {
  int width;
  int height;
  bool can_use_hardware;
  AVCodec* codec;
  AVCodecContext* context;
  AVFrame* sw_frame;
  AVFrame* hw_frame;
  AVBufferRef* ref;
  AVPacket packet;
  enum AVPixelFormat match_fmt;
  DecodeType type;
  enum AVHWDeviceType device_type;
} video_decoder_t;

/*
============================
Public Functions
============================
*/







/**
 * @brief                          This will initialize the FFmpeg AAC audio decoder, and set the proper
 *                                 audio parameters for receiving from the server
 */



video_decoder_t* create_video_decoder(int width, int height, bool use_hardware);

void destroy_video_decoder(video_decoder_t* decoder);

bool video_decoder_decode(video_decoder_t* decoder, void* buffer, int buffer_size);






#endif  // VIDEO_DECODE_H
