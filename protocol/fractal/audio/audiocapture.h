#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audiocapture.h
 * @brief This file contains the code to capture audio on the servers, dynamically toggling Windows and Linux. 
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#ifdef _WIN32
#include "wasapicapture.h"
#else
#include "alsacapture.h"
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @returns                        The initialized audio device struct
 */
audio_device_t* CreateAudioDevice();





/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
void StartAudioDevice(audio_device_t* audio_device);


/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
void DestroyAudioDevice(audio_device_t* audio_device);


/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
void GetNextPacket(audio_device_t* audio_device);


/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
bool PacketAvailable(audio_device_t* audio_device);


/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
void GetBuffer(audio_device_t* audio_device);


/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
void ReleaseBuffer(audio_device_t* audio_device);


/**
 * @brief                          Create an audio device to capture audio on Windows or Linux Ubuntu
 * 
 * @param audio_device             The audio device that gets started to capture audio
 */
void WaitTimer(audio_device_t* audio_device);







#endif  // AUDIO_CAPTURE_H
