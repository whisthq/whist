/*
 * This file contains the headers and definitions of the functions to capture
 * a Windows 10 desktop screen.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef AUDIOCAPTURE_H  // include guard
#define AUDIOCAPTURE_H

#include "fractal.h" // contains all the headers

typedef struct
{
    AVFormatContext *fmt_ctx;
    AVCodecContext *dec_ctx;
    int audio_stream_index;
} audio_capture_device;


// @brief creates a struct device to capture a Windows 10 screen
// @details returns the capture device created for a specific window and area
audio_capture_device *create_audio_capture_device();

// @brief destroy the capture device
// @details deletes the capture device struct
FractalStatus destroy_audio_capture_device(audio_capture_device *device);

#endif // AUDIOCAPTURE_H
