/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file wasapicapture.c
 * @brief This file contains the code to capture audio on Windows using WASAPI.
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

#include <whist/core/platform.h>
#if OS_IS(OS_WIN32)

#include <whist/core/whist.h>
#include "wasapicapture.h"
#include <initguid.h>

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92,
            0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36,
            0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03,
            0xb2);
DEFINE_GUID(IID_IAudioClient3, 0x7ed4ee07, 0x8E67, 0x4CD4, 0x8C, 0x1A, 0x2B, 0x7A, 0x59, 0x87, 0xAD,
            0x42);
DEFINE_GUID(IID_IAudioCaptureClient, 0xc8adbd64, 0xe71e, 0x48a0, 0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c,
            0xd3, 0x17);

/*
============================
Public Function Implementations
============================
*/

AudioDevice *create_audio_device(void) {
    /*
        Create an audio device to capture audio on Windows

        Returns:
            (AudioDevice*): The initialized audio device struct
    */

    AudioDevice *audio_device = safe_malloc(sizeof(AudioDevice));
    memset(audio_device, 0, sizeof(AudioDevice));

    HRESULT hr = CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                          (void **)&audio_device->pMMDeviceEnumerator);
    if (FAILED(hr)) {
        LOG_ERROR("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
        return NULL;
    }

    // get the default render endpoint
    hr = audio_device->pMMDeviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        audio_device->pMMDeviceEnumerator, eRender, eConsole, &audio_device->device);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get default audio endpoint.");
        free(audio_device);
        return NULL;
    }

    hr =
        audio_device->device->lpVtbl->Activate(audio_device->device, &IID_IAudioClient3, CLSCTX_ALL,
                                               NULL, (void **)&audio_device->pAudioClient);
    if (FAILED(hr)) {
        LOG_ERROR("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", hr);
        free(audio_device);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetDevicePeriod(
        audio_device->pAudioClient, &audio_device->hnsDefaultDevicePeriod, NULL);
    if (FAILED(hr)) {
        LOG_ERROR("IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        free(audio_device);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetMixFormat(audio_device->pAudioClient,
                                                          &audio_device->pwfx);
    if (FAILED(hr)) {
        LOG_ERROR("IAudioClient::GetMixFormat failed: hr = 0x%08x", hr);
        free(audio_device);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->Initialize(
        audio_device->pAudioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0,
        audio_device->pwfx, 0);
    if (FAILED(hr)) {
        LOG_ERROR("IAudioClient::Initialize failed: hr = 0x%08x", hr);
        free(audio_device);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetService(
        audio_device->pAudioClient, &IID_IAudioCaptureClient,
        (void **)&audio_device->pAudioCaptureClient);

    if (FAILED(hr)) {
        LOG_ERROR("IAudioClient::GetService failed: hr = 0x%08x", hr);
        free(audio_device);
        return NULL;
    }

    REFERENCE_TIME minimum_period;
    hr = audio_device->pAudioClient->lpVtbl->GetDevicePeriod(audio_device->pAudioClient, NULL,
                                                             &minimum_period);
    if (FAILED(hr)) {
        LOG_ERROR("IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        free(audio_device);
        return NULL;
    }
    LOG_INFO("Minimum period: %d", minimum_period);

    audio_device->sample_rate = audio_device->pwfx->nSamplesPerSec;

    return audio_device;
}

void start_audio_device(AudioDevice *audio_device) {
    /*
        Set the audio device to start capturing audio

        Arguments:
            audio_device (AudioDevice*): The audio device that gets
                started to capture audio
    */

    audio_device->hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);

    LARGE_INTEGER li_first_fire;
    li_first_fire.QuadPart =
        -1 * audio_device->hnsDefaultDevicePeriod / 2;  // negative means relative time
    LONG l_time_between_fires =
        (LONG)audio_device->hnsDefaultDevicePeriod / 2 / 10000;  // convert to milliseconds
    BOOL b_ok = SetWaitableTimer(audio_device->hWakeUp, &li_first_fire, l_time_between_fires, NULL,
                                 NULL, FALSE);
    if (b_ok == 0) {
        LOG_WARNING("Failed to SetWaitableTimer");
        return;
    }

    audio_device->pAudioClient->lpVtbl->Start(audio_device->pAudioClient);
}

void destroy_audio_device(AudioDevice *audio_device) {
    /*
        Stop, release, and destroy the audio device sutrct and free its memory

        Arguments:
            audio_device (AudioDevice*): The audio device that gets destroyed
    */

    audio_device->pAudioClient->lpVtbl->Stop(audio_device->pAudioClient);
    audio_device->pAudioCaptureClient->lpVtbl->Release(audio_device->pAudioCaptureClient);
    CoTaskMemFree(audio_device->pwfx);
    audio_device->pAudioClient->lpVtbl->Release(audio_device->pAudioClient);
    audio_device->device->lpVtbl->Release(audio_device->device);
    audio_device->pMMDeviceEnumerator->lpVtbl->Release(audio_device->pMMDeviceEnumerator);
    CoUninitialize();
    free(audio_device);
}

void get_next_packet(AudioDevice *audio_device) {
    /*
        Request the next packet of audio data from the captured audio stream

        Arguments:
            audio_device (AudioDevice*): The audio device that captures the audio
                stream
    */

    audio_device->hNextPacketResult = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(
        audio_device->pAudioCaptureClient, &audio_device->nNextPacketSize);
}

bool packet_available(AudioDevice *audio_device) {
    /*
        Check if the next packet of audio data is available from the captured audio stream

        Arguments:
            audio_device (AudioDevice*): The audio device that captures the audio
                stream

        Returns:
            (bool): true if the next packet of audio data is available, else false
    */

    return SUCCEEDED(audio_device->hNextPacketResult) && audio_device->nNextPacketSize > 0;
}

void get_buffer(AudioDevice *audio_device) {
    /*
        Get the buffer holding the next packet of audio data from the audio stream.
        Read audio frames into audio device buffer.

        Arguments:
            audio_device (AudioDevice*): The audio device that captures the audio
                stream
    */

    audio_device->pAudioCaptureClient->lpVtbl->GetBuffer(
        audio_device->pAudioCaptureClient, &audio_device->buffer, &audio_device->frames_available,
        &audio_device->dwFlags, NULL, NULL);
    audio_device->buffer_size = audio_device->frames_available * audio_device->pwfx->nBlockAlign;
}

void release_buffer(AudioDevice *audio_device) {
    audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(audio_device->pAudioCaptureClient,
                                                             audio_device->frames_available);
}

void wait_timer(AudioDevice *audio_device) {
    /*
        Wait for next packet

        Arguments:
            audio_device (AudioDevice*): The audio device that captures the audio
                stream
    */

    WaitForSingleObject(audio_device->hWakeUp, INFINITE);
}

#endif  // Windows
