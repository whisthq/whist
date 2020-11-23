#ifndef WASAPICAPTURE_H
#define WASAPICAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file wasapicapture.h
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

/*
============================
Includes
============================
*/

#include <stdio.h>
#include <stdlib.h>

#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

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

typedef struct audio_device_t {
    IMMDevice* device;
    IMMDeviceEnumerator* pMMDeviceEnumerator;
    IAudioClient3* pAudioClient;
    REFERENCE_TIME hnsDefaultDevicePeriod;
    WAVEFORMATEX* pwfx;
    IAudioCaptureClient* pAudioCaptureClient;
    HANDLE hWakeUp;
    BYTE* buffer;
    LONG buffer_size;
    UINT32 nNumFramesToRead;
    UINT32 frames_available;
    DWORD dwWaitResult;
    DWORD dwFlags;
    UINT32 sample_rate;
    UINT32 nNextPacketSize;
    HRESULT hNextPacketResult;
} audio_device_t;

#endif  // WASAPICAPTURE_H
