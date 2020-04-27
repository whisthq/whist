/*
 * Audio capture on Windows.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "wasapicapture.h"

audio_device_t *CreateAudioDevice(audio_device_t *audio_device) {
    HRESULT hr = CoInitialize(NULL);
    memset(audio_device, 0, sizeof(struct audio_device_t));

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator,
                          (void **)&audio_device->pMMDeviceEnumerator);
    if (FAILED(hr)) {
        mprintf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x",
                hr);
        return NULL;
    }

    // get the default render endpoint
    hr = audio_device->pMMDeviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        audio_device->pMMDeviceEnumerator, eRender, eConsole,
        &audio_device->device);
    if (FAILED(hr)) {
        mprintf("Failed to get default audio endpoint.\n");
        return NULL;
    }

    hr = audio_device->device->lpVtbl->Activate(
        audio_device->device, &IID_IAudioClient3, CLSCTX_ALL, NULL,
        (void **)&audio_device->pAudioClient);
    if (FAILED(hr)) {
        mprintf("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetDevicePeriod(
        audio_device->pAudioClient, &audio_device->hnsDefaultDevicePeriod,
        NULL);
    if (FAILED(hr)) {
        mprintf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetMixFormat(
        audio_device->pAudioClient, &audio_device->pwfx);
    if (FAILED(hr)) {
        mprintf("IAudioClient::GetMixFormat failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->Initialize(
        audio_device->pAudioClient, AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, audio_device->pwfx, 0);
    if (FAILED(hr)) {
        mprintf("IAudioClient::Initialize failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = audio_device->pAudioClient->lpVtbl->GetService(
        audio_device->pAudioClient, &IID_IAudioCaptureClient,
        (void **)&audio_device->pAudioCaptureClient);

    if (FAILED(hr)) {
        mprintf("IAudioClient::GetService failed: hr = 0x%08x", hr);
        return NULL;
    }

    REFERENCE_TIME minimum_period;
    hr = audio_device->pAudioClient->lpVtbl->GetDevicePeriod(
        audio_device->pAudioClient, NULL, &minimum_period);
    if (FAILED(hr)) {
        mprintf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        return NULL;
    }
    mprintf("Minimum period: %d\n", minimum_period);

    audio_device->sample_rate = audio_device->pwfx->nSamplesPerSec;

    return audio_device;
}

void StartAudioDevice(audio_device_t *audio_device) {
    audio_device->hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);

    LARGE_INTEGER liFirstFire;
    liFirstFire.QuadPart = -1 * audio_device->hnsDefaultDevicePeriod /
                           2;  // negative means relative time
    LONG lTimeBetweenFires = (LONG)audio_device->hnsDefaultDevicePeriod / 2 /
                             10000;  // convert to milliseconds
    BOOL bOK = SetWaitableTimer(audio_device->hWakeUp, &liFirstFire,
                                lTimeBetweenFires, NULL, NULL, FALSE);
    if (bOK == 0) {
        mprintf("Failed to SetWaitableTimer\n");
        return;
    }

    audio_device->pAudioClient->lpVtbl->Start(audio_device->pAudioClient);
}

void DestroyAudioDevice(audio_device_t *audio_device) {
    audio_device->pAudioClient->lpVtbl->Stop(audio_device->pAudioClient);
    audio_device->pAudioCaptureClient->lpVtbl->Release(
        audio_device->pAudioCaptureClient);
    CoTaskMemFree(audio_device->pwfx);
    audio_device->pAudioClient->lpVtbl->Release(audio_device->pAudioClient);
    audio_device->device->lpVtbl->Release(audio_device->device);
    audio_device->pMMDeviceEnumerator->lpVtbl->Release(
        audio_device->pMMDeviceEnumerator);
    CoUninitialize();
}

void GetNextPacket(audio_device_t *audio_device) {
    audio_device->hNextPacketResult =
        audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(
            audio_device->pAudioCaptureClient, &audio_device->nNextPacketSize);
}

bool PacketAvailable(audio_device_t *audio_device) {
    return SUCCEEDED(audio_device->hNextPacketResult) &&
           audio_device->nNextPacketSize > 0;
}

void GetBuffer(audio_device_t *audio_device) {
    audio_device->pAudioCaptureClient->lpVtbl->GetBuffer(
        audio_device->pAudioCaptureClient, &audio_device->buffer,
        &audio_device->frames_available, &audio_device->dwFlags, NULL, NULL);
    audio_device->buffer_size =
        audio_device->frames_available * audio_device->pwfx->nBlockAlign;
}

void ReleaseBuffer(audio_device_t *audio_device) {
    audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
        audio_device->pAudioCaptureClient, audio_device->frames_available);
}

void WaitTimer(audio_device_t *audio_device) {
    WaitForSingleObject(audio_device->hWakeUp, INFINITE);
}
