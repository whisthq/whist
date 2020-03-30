#include <stdio.h>
#include <stdlib.h>

#include "alsacapture.h"  // header file for this file

audio_device *CreateAudioDevice(audio_device *device) {
    memset(device, 0, sizeof(struct audio_device));

    int res;

    // open pcm device for stream capture
    res = snd_pcm_open(&device->handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (res < 0) {
        mprintf("Failed to open PCM device: %s\n", snd_strerror(res));
        return NULL;
    }

    // allocate params context
    snd_pcm_hw_params_alloca(&device->params);

    res = snd_pcm_hw_params_any(device->handle, device->params);
    if (res < 0) {
        mprintf("No available PCM hardware configurations.\n");
        return NULL;
    }

    // set sample format
    // we should do format cascading selection here and similarly below
    device->sample_format = SND_PCM_FORMAT_FLOAT_LE;
    res = snd_pcm_hw_params_set_format(device->handle, device->params,
                                       device->sample_format);

    if (res < 0) {
        mprintf("PCM sample format 'enum _snd_pcm_format %d' unavailable.\n",
                device->sample_format);
        return NULL;
    }

    // number of channels
    device->channels = 2;
    res = snd_pcm_hw_params_set_channels_near(device->handle, device->params,
                                              &device->channels);
    if (res < 0) {
        mprintf("PCM cannot set format with num channels: %d\n",
                device->channels);
        return NULL;
    }

    // set device to read interleaved samples
    res = snd_pcm_hw_params_set_access(device->handle, device->params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0) {
        mprintf("Unavailable PCM access type.\n");
        return NULL;
    }

    // set stream rate
    device->sample_rate = 44100;
    res = snd_pcm_hw_params_set_rate_near(device->handle, device->params,
                                          &device->sample_rate, 0);
    if (res < 0) {
        mprintf("PCM cannot set format with sample rate: %d\n",
                device->sample_rate);
        return NULL;
    }

    // set frames per period
    device->num_frames = 48;
    res = snd_pcm_hw_params_set_period_size_near(device->handle, device->params,
                                                 &device->num_frames, 0);

    // write parameters according to our configuration space to device (can
    // restrict further if desired)
    res = snd_pcm_hw_params(device->handle, device->params);
    if (res < 0) {
        mprintf("Unable to set hw parameters. Error: %s\n", snd_strerror(res));
        return NULL;
    }

    device->buffer_size = device->num_frames *
                          (snd_pcm_format_width(device->sample_format) / 8) *
                          device->channels;
    device->buffer = (uint8_t *)malloc(device->buffer_size);

    return device;
}

void StartAudioDevice(audio_device *device) {
    device->dummy_state = 0;
    return;
}

void DestroyAudioDevice(audio_device *device) {
    snd_pcm_drop(device->handle);
    snd_pcm_close(device->handle);
    free(device->buffer);
    free(device);
}

void GetNextPacket(audio_device *device) {
    device->dummy_state++;
    return;
}

// make it so the for loop only happens once for ALSA (unlike WASAPI)
bool PacketAvailable(audio_device *device) { return device->dummy_state < 2; }

void GetBuffer(audio_device *device) {
    int res = snd_pcm_readi(device->handle, device->buffer, device->num_frames);
    if (res == -EPIPE) {
        snd_pcm_recover(device->handle, res, 0);
    } else if (res < 0) {
        mprintf("Error from PCM read: %s\n", snd_strerror(res));
    } else {
        device->frames_available = res;
    }
}

void ReleaseBuffer(audio_device *device) { return; }

// ALSA is blocking, unlike WASAPI
void WaitTimer(audio_device *device) { return; }