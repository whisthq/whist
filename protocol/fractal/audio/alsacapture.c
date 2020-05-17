/*
 * Audio capture on Linux Ubuntu.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "alsacapture.h"

audio_device_t *CreateAudioDevice() {
    audio_device_t* audio_device = malloc( sizeof( audio_device_t ) );
    memset(audio_device, 0, sizeof(audio_device_t));

    int res;

    // open pcm device for stream capture
    res = snd_pcm_open(&audio_device->handle, "default", SND_PCM_STREAM_CAPTURE,
                       0);
    if (res < 0) {
        LOG_WARNING("Failed to open PCM device: %s\n", snd_strerror(res));
        free( audio_device );
        return NULL;
    }

    // allocate params context
    snd_pcm_hw_params_alloca(&audio_device->params);

    res = snd_pcm_hw_params_any(audio_device->handle, audio_device->params);
    if (res < 0) {
        LOG_WARNING("No available PCM hardware configurations.\n");
        free( audio_device );
        return NULL;
    }

    // set sample format
    // we should do format cascading selection here and similarly below
    audio_device->sample_format = SND_PCM_FORMAT_FLOAT_LE;
    res =
        snd_pcm_hw_params_set_format(audio_device->handle, audio_device->params,
                                     audio_device->sample_format);

    if (res < 0) {
        LOG_WARNING("PCM sample format 'enum _snd_pcm_format %d' unavailable.\n",
                audio_device->sample_format);
        free( audio_device );
        return NULL;
    }

    // number of channels
    audio_device->channels = 2;
    res = snd_pcm_hw_params_set_channels_near(
        audio_device->handle, audio_device->params, &audio_device->channels);
    if (res < 0) {
        LOG_WARNING("PCM cannot set format with num channels: %d\n",
                audio_device->channels);
        free( audio_device );
        return NULL;
    }

    // set device to read interleaved samples
    res =
        snd_pcm_hw_params_set_access(audio_device->handle, audio_device->params,
                                     SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0) {
        LOG_WARNING("Unavailable PCM access type.\n");
        free( audio_device );
        return NULL;
    }

    // set stream rate
    audio_device->sample_rate = 44100;
    res = snd_pcm_hw_params_set_rate_near(audio_device->handle,
                                          audio_device->params,
                                          &audio_device->sample_rate, 0);
    if (res < 0) {
        LOG_WARNING("PCM cannot set format with sample rate: %d\n",
                audio_device->sample_rate);
        free( audio_device );
        return NULL;
    }

    // set frames per period
    audio_device->num_frames = 120;
    res = snd_pcm_hw_params_set_period_size_near(audio_device->handle,
                                                 audio_device->params,
                                                 &audio_device->num_frames, 0);

    // write parameters according to our configuration space to device (can
    // restrict further if desired)
    res = snd_pcm_hw_params(audio_device->handle, audio_device->params);

    if (res < 0) {
        LOG_WARNING("Unable to set hw parameters. Error: %s\n", snd_strerror(res));
        free( audio_device );
        return NULL;
    }

    audio_device->frame_size =
        (snd_pcm_format_width(audio_device->sample_format) / 8) *
        audio_device->channels;
    audio_device->buffer_size =
        audio_device->num_frames * audio_device->frame_size;
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
bool PacketAvailable(audio_device_t *audio_device) {
    return audio_device->dummy_state < 2;
}

void GetBuffer(audio_device_t *audio_device) {
    int res = snd_pcm_readi(audio_device->handle, audio_device->buffer,
                            audio_device->num_frames);
    if (res == -EPIPE) {
        snd_pcm_recover(audio_device->handle, res, 0);
        audio_device->frames_available = 0;
    } else if (res < 0) {
        LOG_WARNING("Error from PCM read: %s\n", snd_strerror(res));
        snd_pcm_recover(audio_device->handle, res, 0);
        audio_device->frames_available = 0;
    } else {
        audio_device->frames_available = res;
    }
    audio_device->buffer_size =
        audio_device->frames_available * audio_device->frame_size;
}

void ReleaseBuffer(audio_device_t *audio_device) { return; }

// ALSA is blocking, unlike WASAPI
void WaitTimer(audio_device_t *audio_device) { audio_device->dummy_state = 0; }
