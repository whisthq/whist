/**
 * Copyright Fractal Computers, Inc. 2020
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

#include "wasapicapture.h"

audio_device_t *CreateAudioDevice() {
    audio_device_t *audio_device = malloc(sizeof(audio_device_t));
    memset(audio_device, 0, sizeof(audio_device_t));

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
        LOG_ERROR("Failed to get default audio endpoint.\n");
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
    LOG_INFO("Minimum period: %d\n", minimum_period);

    audio_device->sample_rate = audio_device->pwfx->nSamplesPerSec;

    return audio_device;
}

void StartAudioDevice(audio_device_t *audio_device) {
    audio_device->hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);

    LARGE_INTEGER liFirstFire;
    liFirstFire.QuadPart =
        -1 * audio_device->hnsDefaultDevicePeriod / 2;  // negative means relative time
    LONG lTimeBetweenFires =
        (LONG)audio_device->hnsDefaultDevicePeriod / 2 / 10000;  // convert to milliseconds
    BOOL bOK =
        SetWaitableTimer(audio_device->hWakeUp, &liFirstFire, lTimeBetweenFires, NULL, NULL, FALSE);
    if (bOK == 0) {
        LOG_WARNING("Failed to SetWaitableTimer\n");
        return;
    }

    audio_device->pAudioClient->lpVtbl->Start(audio_device->pAudioClient);
}

void DestroyAudioDevice(audio_device_t *audio_device) {
    audio_device->pAudioClient->lpVtbl->Stop(audio_device->pAudioClient);
    audio_device->pAudioCaptureClient->lpVtbl->Release(audio_device->pAudioCaptureClient);
    CoTaskMemFree(audio_device->pwfx);
    audio_device->pAudioClient->lpVtbl->Release(audio_device->pAudioClient);
    audio_device->device->lpVtbl->Release(audio_device->device);
    audio_device->pMMDeviceEnumerator->lpVtbl->Release(audio_device->pMMDeviceEnumerator);
    CoUninitialize();
    free(audio_device);
}

void GetNextPacket(audio_device_t *audio_device) {
    audio_device->hNextPacketResult = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(
        audio_device->pAudioCaptureClient, &audio_device->nNextPacketSize);
}

bool PacketAvailable(audio_device_t *audio_device) {
    return SUCCEEDED(audio_device->hNextPacketResult) && audio_device->nNextPacketSize > 0;
}

void GetBuffer(audio_device_t *audio_device) {
    audio_device->pAudioCaptureClient->lpVtbl->GetBuffer(
        audio_device->pAudioCaptureClient, &audio_device->buffer, &audio_device->frames_available,
        &audio_device->dwFlags, NULL, NULL);
    audio_device->buffer_size = audio_device->frames_available * audio_device->pwfx->nBlockAlign;
}

void ReleaseBuffer(audio_device_t *audio_device) {
    audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(audio_device->pAudioCaptureClient,
                                                             audio_device->frames_available);
}

void WaitTimer(audio_device_t *audio_device) {
    WaitForSingleObject(audio_device->hWakeUp, INFINITE);
}
