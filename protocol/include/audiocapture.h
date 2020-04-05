#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#ifdef _WIN32
#include "../include/wasapicapture.h"
#else
#include "../include/alsacapture.h"
#endif

audio_device *CreateAudioDevice(audio_device *device);

void StartAudioDevice(audio_device *device);

void DestroyAudioDevice(audio_device *device);

void GetNextPacket(audio_device *device);

bool PacketAvailable(audio_device *device);

void GetBuffer(audio_device *device);

void ReleaseBuffer(audio_device *device);

void WaitTimer(audio_device *device);

#endif  // AUDIO_CAPTURE_H