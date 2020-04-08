#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#ifdef _WIN32
#include "../include/wasapicapture.h"
#else
#include "../include/alsacapture.h"
#endif

audio_device_t *CreateAudioDevice(audio_device_t *audio_device);

void StartAudioDevice(audio_device_t *audio_device);

void DestroyAudioDevice(audio_device_t *audio_device);

void GetNextPacket(audio_device_t *audio_device);

bool PacketAvailable(audio_device_t *audio_device);

void GetBuffer(audio_device_t *audio_device);

void ReleaseBuffer(audio_device_t *audio_device);

void WaitTimer(audio_device_t *audio_device);

#endif  // AUDIO_CAPTURE_H
