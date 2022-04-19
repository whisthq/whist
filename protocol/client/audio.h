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
 * @brief                          Render the audio frame (If any are available to render)
 *
 * @param audio_context            The audio context that potentially wants to render audio
 *
 * @note                           This function is thread-safe, and may be called
 *                                 independently of the rest of the functions
 */
void render_audio(AudioContext* audio_context);

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

#endif  // CLIENT_AUDIO_H
