#ifndef WASAPICAPTURE_H
#define WASAPICAPTURE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include <whist/core/whist.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#include <stdio.h>
#include <stdlib.h>

/*
============================
Custom Types
============================
*/

/**
 * @brief                          Audio capture device features
 */
typedef struct AudioDevice {
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
} AudioDevice;

#endif  // WASAPICAPTURE_H
