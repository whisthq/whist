#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audiocapture.h
 * @brief This file contains the code to capture audio on the servers,
 *        dynamically toggling Windows and Linux.
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
 * @brief                          Create an audio device to capture audio on
 *                                 Windows or Linux Ubuntu
 *
 * @returns                        The initialized audio device struct
 */
audio_device_t* CreateAudioDevice();

/**
 * @brief                          Set the audio device to start capturing audio
 *
 * @param audio_device             The audio device that gets started to capture
 *                                 audio
 */
void StartAudioDevice(audio_device_t* audio_device);

/**
 * @brief                          Destroy the audio device sutrct and free its
 *                                 memory
 *
 * @param audio_device             The audio device that gets destroyed
 */
void DestroyAudioDevice(audio_device_t* audio_device);

/**
 * @brief                          Request the next packet of audio data from
 *                                 the captured audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void GetNextPacket(audio_device_t* audio_device);

/**
 * @brief                          Check if the next packet of audio data is
 *                                 available from the captured audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 *
 * @returns                        True if the next packet of audio data is
 *                                 available, else False
 */
bool PacketAvailable(audio_device_t* audio_device);

/**
 * @brief                          Get the buffer holding the next packet of
 *                                 audio data from the audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void GetBuffer(audio_device_t* audio_device);

/**
 * @brief                          Release the buffer holding the next packet of
 *                                 audio data from the audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void ReleaseBuffer(audio_device_t* audio_device);

/**
 * @brief                          Wait for the next packet (only on ALSA, since
 *                                 it is blocking, while WASAPI is not)
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void WaitTimer(audio_device_t* audio_device);

#endif  // AUDIO_CAPTURE_H
