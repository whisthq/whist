#include <stdio.h>
#include <stdlib.h>

#include "wasapicapture.h"  // header file for this file

audio_device *CreateAudioDevice(audio_device *device) {
    HRESULT hr = CoInitialize(NULL);
    memset(device, 0, sizeof(struct audio_device));

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator,
                          (void **)&device->pMMDeviceEnumerator);
    if (FAILED(hr)) {
        mprintf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x",
                hr);
        return NULL;
    }

    // get the default render endpoint
    hr = device->pMMDeviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        device->pMMDeviceEnumerator, eRender, eConsole, &device->device);
    if (FAILED(hr)) {
        mprintf("Failed to get default audio endpoint.\n");
        return NULL;
    }

    hr = device->device->lpVtbl->Activate(device->device, &IID_IAudioClient3,
                                          CLSCTX_ALL, NULL,
                                          (void **)&device->pAudioClient);
    if (FAILED(hr)) {
        mprintf("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = device->pAudioClient->lpVtbl->GetDevicePeriod(
        device->pAudioClient, &device->hnsDefaultDevicePeriod, NULL);
    if (FAILED(hr)) {
        mprintf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = device->pAudioClient->lpVtbl->GetMixFormat(device->pAudioClient,
                                                    &device->pwfx);
    if (FAILED(hr)) {
        mprintf("IAudioClient::GetMixFormat failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = device->pAudioClient->lpVtbl->Initialize(
        device->pAudioClient, AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, device->pwfx, 0);
    if (FAILED(hr)) {
        mprintf("IAudioClient::Initialize failed: hr = 0x%08x", hr);
        return NULL;
    }

    hr = device->pAudioClient->lpVtbl->GetService(
        device->pAudioClient, &IID_IAudioCaptureClient,
        (void **)&device->pAudioCaptureClient);

    if (FAILED(hr)) {
        mprintf("IAudioClient::GetService failed: hr = 0x%08x", hr);
        return NULL;
    }

    REFERENCE_TIME minimum_period;
    hr = device->pAudioClient->lpVtbl->GetDevicePeriod(device->pAudioClient,
                                                       NULL, &minimum_period);
    if (FAILED(hr)) {
        mprintf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x", hr);
        return NULL;
    }
    mprintf("Minimum period: %d\n", minimum_period);

    device->sample_rate = device->pwfx->nSamplesPerSec;

    return device;
}

void StartAudioDevice(audio_device *device) {
    device->hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);

    LARGE_INTEGER liFirstFire;
    liFirstFire.QuadPart = -1 * device->hnsDefaultDevicePeriod /
                           2;  // negative means relative time
    LONG lTimeBetweenFires = (LONG)device->hnsDefaultDevicePeriod / 2 /
                             10000;  // convert to milliseconds
    BOOL bOK = SetWaitableTimer(device->hWakeUp, &liFirstFire,
                                lTimeBetweenFires, NULL, NULL, FALSE);
    if (bOK == 0) {
        mprintf("Failed to SetWaitableTimer\n");
        return;
    }

    device->pAudioClient->lpVtbl->Start(device->pAudioClient);
}

void DestroyAudioDevice(audio_device *device) {
    device->pAudioClient->lpVtbl->Stop(device->pAudioClient);
    device->pAudioCaptureClient->lpVtbl->Release(device->pAudioCaptureClient);
    CoTaskMemFree(device->pwfx);
    device->pAudioClient->lpVtbl->Release(device->pAudioClient);
    device->device->lpVtbl->Release(device->device);
    device->pMMDeviceEnumerator->lpVtbl->Release(device->pMMDeviceEnumerator);
    CoUninitialize();
}

void GetNextPacket(audio_device *device) {
    device->hNextPacketResult =
        device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(
            device->pAudioCaptureClient, &device->nNextPacketSize);
}

bool PacketAvailable(audio_device *device) {
    return SUCCEEDED(device->hNextPacketResult) && device->nNextPacketSize > 0;
}

void GetBuffer(audio_device *device) {
    device->pAudioCaptureClient->lpVtbl->GetBuffer(
        device->pAudioCaptureClient, &device->buffer, &device->nNumFramesToRead,
        &device->dwFlags, NULL, NULL);
    device->buffer_size = device->nNumFramesToRead * device->pwfx->nBlockAlign;
}

void ReleaseBuffer(audio_device *device) {
    device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
        device->pAudioCaptureClient, device->nNumFramesToRead);
}

void WaitTimer(audio_device *device) {
    WaitForSingleObject(device->hWakeUp, INFINITE);
}