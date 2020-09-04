/**
 * Copyright Fractal Computers, Inc. 2020
 * @file alsacapture.c
 * @brief This file contains the code to capture audio on Ubuntu using ALSA.
============================
Usage
============================

Audio is captured as a stream. You first need to create an audio device via
CreateAudioDevice. You can then start it via StartAudioDevice and it will
capture all audio data it finds. It captures nothing if there is no audio
playing. You can use GetNextPacket to retrieve the next packet of audio data
from the stream,, and GetBuffer to grab the data. Packets will keep coming
whether you grab them or not. Once you are done, you need to destroy the audio
device via DestroyAudioDevice.
*/

#include "alsacapture.h"

/*
============================
Public Functions
============================
*/

audio_device_t *CreateAudioDevice() {
    // See http://alsamodular.sourceforge.net/alsa_programming_howto.html

    audio_device_t *audio_device = malloc(sizeof(audio_device_t));
    memset(audio_device, 0, sizeof(audio_device_t));

    int res;

    // open pcm device for stream capture
    res = snd_pcm_open(&audio_device->handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (res < 0) {
        LOG_WARNING("Failed to open PCM device: %s", snd_strerror(res));
        free(audio_device);
        return NULL;
    }

    snd_pcm_hw_params_t *params;

    // allocate params context
    snd_pcm_hw_params_malloc(&params);

    res = snd_pcm_hw_params_any(audio_device->handle, params);
    if (res < 0) {
        LOG_WARNING("No available PCM hardware configurations.");
        snd_pcm_close(audio_device->handle);
        free(audio_device);
        return NULL;
    }

#define FREE_ALL()                           \
    {                                        \
        snd_pcm_close(audio_device->handle); \
        free(audio_device);                  \
        snd_pcm_hw_params_free(params);      \
    };

    // set sample format
    // we should do format cascading selection here and similarly below
    audio_device->sample_format = SND_PCM_FORMAT_FLOAT_LE;
    res = snd_pcm_hw_params_set_format(audio_device->handle, params, audio_device->sample_format);

    if (res < 0) {
        LOG_WARNING("PCM sample format 'enum _snd_pcm_format %d' unavailable.",
                    audio_device->sample_format);
        FREE_ALL();
        return NULL;
    }

    // number of channels
    audio_device->channels = 2;
    res =
        snd_pcm_hw_params_set_channels_near(audio_device->handle, params, &audio_device->channels);
    if (res < 0) {
        LOG_WARNING("PCM cannot set format with num channels: %d", audio_device->channels);
        FREE_ALL();
        return NULL;
    }

    // set device to read interleaved samples
    res = snd_pcm_hw_params_set_access(audio_device->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0) {
        LOG_WARNING("Unavailable PCM access type.");
        FREE_ALL();
        return NULL;
    }

    res = snd_pcm_hw_params_set_rate_resample(audio_device->handle, params, 0);
    if (res < 0) {
        LOG_WARNING("PCM cannot set resample");
        FREE_ALL();
        return NULL;
    }

    // set stream rate
    audio_device->sample_rate = 44100;  // Hertz
    int dir = 0;
    res = snd_pcm_hw_params_set_rate_near(audio_device->handle, params, &audio_device->sample_rate,
                                          &dir);
    if (res < 0) {
        LOG_WARNING("PCM cannot set format with sample rate: %d", audio_device->sample_rate);
        FREE_ALL();
        return NULL;
    }

    // set frames per period
    audio_device->num_frames = 8;
    res = snd_pcm_hw_params_set_period_size_near(audio_device->handle, params,
                                                 &audio_device->num_frames, 0);

    if (res < 0) {
        LOG_WARNING("PCM cannot set period: %s", snd_strerror(res));
        FREE_ALL();
        return NULL;
    }

    audio_device->frame_size =
        (snd_pcm_format_width(audio_device->sample_format) / 8) * audio_device->channels;
    audio_device->buffer_size = audio_device->num_frames * audio_device->frame_size;

    res = snd_pcm_hw_params_set_buffer_size_near(audio_device->handle, params,
                                                 &audio_device->buffer_size);
    if (res < 0) {
        LOG_WARNING("PCM Error setting buffersize: [%s]\n", snd_strerror(res));
        FREE_ALL();
        return NULL;
    }

    // write parameters according to our configuration space to device (can
    // restrict further if desired)
    res = snd_pcm_hw_params(audio_device->handle, params);

    if (res < 0) {
        LOG_WARNING("Unable to set hw parameters. Error: %s", snd_strerror(res));
        FREE_ALL();
        return NULL;
    }

    snd_pcm_hw_params_free(params);

    audio_device->buffer = (uint8_t *)malloc(audio_device->buffer_size);

    return audio_device;
}

void StartAudioDevice(audio_device_t *audio_device) {
    audio_device->dummy_state = 0;
    return;
}

void DestroyAudioDevice(audio_device_t *audio_device) {
    snd_pcm_drop(audio_device->handle);
    snd_pcm_close(audio_device->handle);
    free(audio_device->buffer);
    free(audio_device);
}

void GetNextPacket(audio_device_t *audio_device) {
    audio_device->dummy_state++;
    return;
}

// make it so the for loop only happens once for ALSA (unlike WASAPI)
bool PacketAvailable(audio_device_t *audio_device) { return audio_device->dummy_state < 2; }

void GetBuffer(audio_device_t *audio_device) {
    int res = snd_pcm_readi(audio_device->handle, audio_device->buffer, audio_device->num_frames);
    if (res == -EPIPE) {
        snd_pcm_recover(audio_device->handle, res, 0);
        audio_device->frames_available = 0;
    } else if (res < 0) {
        LOG_WARNING("Error from PCM read: %s", snd_strerror(res));
        snd_pcm_recover(audio_device->handle, res, 0);
        audio_device->frames_available = 0;
    } else {
        audio_device->frames_available = res;
    }
    audio_device->buffer_size = audio_device->frames_available * audio_device->frame_size;
}

void ReleaseBuffer(audio_device_t *audio_device) { return; }

// ALSA is blocking, unlike WASAPI
void WaitTimer(audio_device_t *audio_device) { audio_device->dummy_state = 0; }
