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

typedef struct {
    VideoContext* video_context;
    AudioContext* audio_context;

    bool run_renderer_thread;
    WhistThread renderer_thread;
    WhistMutex renderer_mutex;
    WhistSemaphore renderer_semaphore;
    clock last_try_render_timer;
    // Used to log renderer thread usage
    bool using_renderer_thread;
    bool render_is_on_renderer_thread;
} WhistRenderer;

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
 * @brief                          Receive video or audio packet
 *
 * @param renderer                 The renderer context to give a packet to
 *
 * @param packet                   Packet as received from the server
 *
 * @note                           This function is guaranteed to return virtually instantly.
 *                                 It may be used in any hotpaths.
 */
void renderer_receive_packet(WhistRenderer* renderer, WhistPacket* packet);

/**
 * @brief                          Render the video frame (If any are available to render)
 *
 * @param renderer                 The video context that wants to render a frame
 *
 * @note                           This function is thread-safe, and may be called
 *                                 independently of receive_packet/update
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
