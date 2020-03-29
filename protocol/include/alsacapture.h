#ifndef ALSACAPTURE_H
#define ALSACAPTURE_H

#include <alsa/asoundlib.h>
#include "fractal.h"

typedef struct audio_device {
    snd_pcm_t *handle;
    snd_pcm_uframes_t num_frames;
    unsigned long frames_available;
    unsigned long buffer_size;
    unsigned int channels;
    unsigned int sample_rate;
    enum _snd_pcm_format sample_format;
    uint8_t *buffer;
    snd_pcm_hw_params_t *params;
    bool dummy_state;
} audio_device;

#endif  // ALSA_CAPTURE_H