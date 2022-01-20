#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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
AudioDevice* create_audio_device(void);

/**
 * @brief                          Set the audio device to start capturing audio
 *
 * @param audio_device             The audio device that gets started to capture
 *                                 audio
 */
void start_audio_device(AudioDevice* audio_device);

/**
 * @brief                          Destroy the audio device sutrct and free its
 *                                 memory
 *
 * @param audio_device             The audio device that gets destroyed
 */
void destroy_audio_device(AudioDevice* audio_device);

/**
 * @brief                          Request the next packet of audio data from
 *                                 the captured audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void get_next_packet(AudioDevice* audio_device);

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
bool packet_available(AudioDevice* audio_device);

/**
 * @brief                          Get the buffer holding the next packet of
 *                                 audio data from the audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void get_buffer(AudioDevice* audio_device);

/**
 * @brief                          Release the buffer holding the next packet of
 *                                 audio data from the audio stream
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void release_buffer(AudioDevice* audio_device);

/**
 * @brief                          Wait for the next packet (only on ALSA, since
 *                                 it is blocking, while WASAPI is not)
 *
 * @param audio_device             The audio device that captures the audio
 *                                 stream
 */
void wait_timer(AudioDevice* audio_device);

#endif  // AUDIO_CAPTURE_H
