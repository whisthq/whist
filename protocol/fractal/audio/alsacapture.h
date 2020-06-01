#ifndef ALSACAPTURE_H
#define ALSACAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file alsacapture.h
 * @brief This file contains the code to capture audio on Ubuntu using ALSA.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

typedef struct audio_device_t {
    snd_pcm_t* handle;
    snd_pcm_uframes_t num_frames;
    unsigned long frames_available;
    unsigned long buffer_size;
    unsigned long frame_size;
    unsigned int channels;
    unsigned int sample_rate;
    enum _snd_pcm_format sample_format;
    uint8_t* buffer;
    snd_pcm_hw_params_t* params;
    int dummy_state;
} audio_device_t;

#endif  // ALSA_CAPTURE_H
