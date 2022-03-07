#ifndef CLIENT_AUDIO_H
#define CLIENT_AUDIO_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file audio.h
 * @brief This file contains all code that interacts directly with processing
 *        audio packets on the client.
============================
Usage
============================

See video.h for similar usage
*/

/*
============================
Includes
============================
*/

#include <whist/audio/audiodecode.h>
#include <whist/core/whist.h>
#include <whist/network/network.h>
#include "frontend/frontend.h"
#include "client_utils.h"

/*
============================
Defines
============================
*/

typedef struct AudioContext AudioContext;

// TODO: Automatically deduce this from (ms_per_frame / MS_IN_SECOND) * audio_frequency
// TODO: Add ms_per_frame to AudioFrame*
#define SAMPLES_PER_FRAME 480
#define BYTES_PER_SAMPLE 4
#define NUM_CHANNELS 2
#define DECODED_BYTES_PER_FRAME (SAMPLES_PER_FRAME * BYTES_PER_SAMPLE * NUM_CHANNELS)

/*
============================
Public Functions
============================
*/

/**
 * @brief                          This will initialize the audio system.
 *                                 The audio system will receive audio packets,
 *                                 and render the audio out to a playback device
 *
 * @param frontend                 The frontend for which to initialize audio
 *
 * @returns                        The new audio context
 */
AudioContext* init_audio(WhistFrontend* frontend);

/**
 * @brief                          This will refresh the audio device
 *                                 of the audio context prior to the next render.
 *                                 This must be called if a new playback device
 *                                 is plugged in or unplugged.
 *
 * @param audio_context            The audio context to use
 *
 * @note                           This function is thread-safe, and may be
 *                                 called independently of the rest of the functions
 */
void refresh_audio_device(AudioContext* audio_context);

/**
 * @brief                          Receive audio packet
 *
 * @param audio_context            The audio context to give the audio frame to
 *
 * @param audio_frame              Audio Frame to give to the audio context
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 */
void receive_audio(AudioContext* audio_context, AudioFrame* audio_frame);

/**
 * @brief                          Does any pending work the audio context
 *                                 wants to do. (Including decoding frames,
 *                                 and calculating statistics)
 *
 * @param audio_context            The audio context to update
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 *
 * @note                           In order for audio to be responsive,
 *                                 this function *MUST* be called in a tight loop,
 *                                 at least once every millisecond.
 */
void update_audio(AudioContext* audio_context);

/**
 * @brief                          Render the audio frame (If any are available to render)
 *
 * @param audio_context            The audio context that potentially wants to render audio
 *
 * @note                           This function is thread-safe, and may be called
 *                                 independently of the rest of the functions
 */
int render_audio(AudioContext* audio_context);

/**
 * @brief                          Destroy the audio context
 *
 * @param audio_context            The audio context to destroy
 */
void destroy_audio(AudioContext* audio_context);

/**
 * @brief                          Whether or not the audio context is ready to render a new frame
 *
 * @param audio_context            The audio context to query
 * @param num_frames_buffered      The number of frames buffered and waiting to be given
 */
bool audio_ready_for_frame(AudioContext* audio_context, int num_frames_buffered);

// get the lenght of the device audio queue inside audio_context
// unit: bytes
int get_device_audio_queue_bytes(AudioContext* audio_context);
#endif  // CLIENT_AUDIO_H
