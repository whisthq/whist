/*
 * This file contains the implementation of the functions to capture Windows audio.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdio.h>
#include <stdlib.h>

#include "audiocapture.h" // header file for this file

// @brief creates a struct device to capture audio
// @details returns the capture device created for a specific window and area
audio_capture_device *create_audio_capture_device()
{
    av_register_all();
    avcodec_register_all();
    avdevice_register_all();
    avfilter_register_all();

    audio_capture_device *device = (audio_capture_device *) malloc(sizeof(audio_capture_device));
    memset(device, 0, sizeof(audio_capture_device));

    AVCodec *dec;
    AVDictionary *inOptions = NULL;
    AVInputFormat *inFrmt= av_find_input_format("dshow");

    if((avformat_open_input(&device->fmt_ctx, "audio=virtual-audio-capturer", inFrmt, &inOptions)) < 0) {
        printf("Cannot open audio capture device\n");
        exit(1);
    }
    if(avformat_find_stream_info(device->fmt_ctx, NULL) < 0) {
        printf("Cannot find stream info\n");
        exit(1);
    }
    if((device->audio_stream_index = av_find_best_stream(device->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0)) < 0) {
        printf("Cannot find a audio stream\n");
        exit(1);
    }

    device->dec_ctx = device->fmt_ctx->streams[device->audio_stream_index]->codec;
    av_opt_set_int(device->dec_ctx, "refcounted_frames", 1, 0);

    if (avcodec_open2(device->dec_ctx, dec, NULL) < 0) {
        printf("Cannot open audio decoder\n");
        return -1;
    }

    return device;
}

// @brief destroy the capture device
// @details deletes the capture device struct
FractalStatus destroy_audio_capture_device(audio_capture_device *device) {
  // check if capture device exists
  // return success
  avcodec_free_context(&device->dec_ctx);
  free(device);
  return FRACTAL_OK;
}

