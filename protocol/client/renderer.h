#ifndef CLIENT_RENDERER_H
#define CLIENT_RENDERER_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file renderer.h
 * @brief This file contains all code that interacts
 *        with receiving and rendering video/audio packets
============================
Usage
============================

NOTE: Only ONE thread can call a function on whist renderer at a time,
      unless a function says otherwise.

See video.h for similar usage
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>
#include "audio.h"
#include "video.h"

/*
============================
Defines
============================
*/

typedef struct WhistRenderer WhistRenderer;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          This will manage the video and audio systems,
 *                                 so that we can give it packets and it will render
 *                                 video to the screen and audio to the playback device
 *
 * @returns                        The newly created whist renderer
 */
WhistRenderer* init_renderer();

/**
 * @brief                          Does any pending work the renderer
 *                                 wants to do.
 *
 * @param renderer                 The renderer to update
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 *
 * @note                           In order for video and audio to be responsive,
 *                                 this function *MUST* be called in a tight loop,
 *                                 at least once every millisecond.
 */
void renderer_update(WhistRenderer* renderer);

/**
 * @brief                          Whether or not the renderer wants a frame of that type
 *
 * @param renderer                 The renderer context to give a packet to
 *
 * @param packet_type              The type of packet that the renderer may or may not want
 *
 * @param num_buffered_frames      The number of frames of type packet_type,
 *                                 that are waiting in the network buffer
 *
 * @returns                        True if the renderer wants a frame of that type,
 *                                 False otherwise
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 */
bool renderer_wants_frame(WhistRenderer* renderer, WhistPacketType packet_type,
                          int num_buffered_frames);

/**
 * @brief                          Receive video or audio frame
 *
 * @param renderer                 The renderer context to give a frame to
 *
 * @param packet_type              Packet type, either Video or Audio
 *
 * @param frame                    The VideoFrame* / AudioFrame*
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 *
 * @note                           The whist_renderer needs you to keep the data in frame
 *                                 alive, until renderer_wants_frame later returns True,
 *                                 since that implies it's done with the old packet and wants a new
 * one
 *                                 TODO: Use a memcpy to simplify this logic
 */
void renderer_receive_frame(WhistRenderer* renderer, WhistPacketType packet_type, void* frame);

/**
 * @brief                          Render the video/audio frames (If any are available to render)
 *
 * @param renderer                 The video context that wants to render a frame
 *
 * @note                           This function is thread-safe, and may be called in a way
 *                                 that overlaps other functions that use the whist renderer.
 *
 * @note                           This function will potentially take a long time (4-8ms)
 *
 * @note                           This function is optional to call, but it's provided
 *                                 so that rendering work can be done on a thread that
 *                                 already exists. If you fail to call this function,
 *                                 another thread will be used automatically, increasing CPU usage.
 */
void renderer_try_render(WhistRenderer* renderer);

/**
 * @brief                          Destroy the given whist renderer
 *
 * @param renderer                 The whist renderer to destroy
 */
void destroy_renderer(WhistRenderer* renderer);

#endif
