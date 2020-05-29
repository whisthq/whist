#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audiocapture.h






 * 
 * 
 * @brief This file contains the code to create a decoder and use that decoder to decode frames.
============================
Usage
============================
*/




/*
 * General audio capture integrating Windows and Linux Ubuntu servers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/


#ifdef _WIN32
#include "wasapicapture.h"
#else
#include "alsacapture.h"
#endif

audio_device_t* CreateAudioDevice();

void StartAudioDevice(audio_device_t* audio_device);

void DestroyAudioDevice(audio_device_t* audio_device);

void GetNextPacket(audio_device_t* audio_device);

bool PacketAvailable(audio_device_t* audio_device);

void GetBuffer(audio_device_t* audio_device);

void ReleaseBuffer(audio_device_t* audio_device);

void WaitTimer(audio_device_t* audio_device);







#endif  // AUDIO_CAPTURE_H
