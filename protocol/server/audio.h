#ifndef SERVER_AUDIO_H
#define SERVER_AUDIO_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file audio.h
 * @brief This file contains all code that interacts directly with processing
 *        audio on the server.
============================
Usage
============================

multithreaded_send_audio() is called on its own thread and loops repeatedly to capture and send
audio.
*/

/*
============================
Public Functions
============================
*/

/**
 * @brief                          This loops and collects audio samples, encodes
 *                                 the frames, and sends them to the client
 */
int32_t multithreaded_send_audio(void* opaque);

#endif  // SERVER_AUDIO_H
