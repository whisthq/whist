/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file renderer.c
 * @brief This file contains the video/audio renderer
 */

/*
============================
Includes
============================
*/

#include "renderer.h"
#include "handle_frontend_events.h"
#include "whist/debug/debug_console.h"
#include <whist/logging/log_statistic.h>

/*
============================
Defines
============================
*/

#define SINGLE_THREAD_MODEL false

struct WhistRenderer {
    VideoContext* video_context;
    AudioContext* audio_context;

    bool run_renderer_threads;

    // Single-thread model
    WhistThread renderer_thread;
    WhistMutex renderer_mutex;
    WhistSemaphore renderer_semaphore;
    WhistTimer last_try_render_timer;
    // Used to log renderer thread usage
    bool using_renderer_thread;
    bool render_is_on_renderer_thread;

    // Multi-thread model
    bool has_video_rendered_yet;
    WhistSemaphore video_semaphore;
    WhistSemaphore audio_semaphore;

    WhistMutex audio_mutex;

    WhistThread video_thread;
    WhistThread audio_thread;
};

#define LOG_RENDERER_THREAD_USAGE false

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Renders audio/video on a separate thread,
 *                                 if necessary
 *
 * @note                           This will be used if `renderer_try_render`
 *                                 doesn't get called often enough otherwise.
 */
int32_t multithreaded_renderer(void* opaque);

/**
 * @brief                          Renders video whenever video_semaphore is posted
 *
 * @param opaque                   The WhistRenderer, as a WhistRenderer*
 */
int32_t multithreaded_video_renderer(void* opaque);

/**
 * @brief                          Renders audio whenever audio_semaphore is posted
 *
 * @param opaque                   The WhistRenderer, as a WhistRenderer*
 */
int32_t multithreaded_audio_renderer(void* opaque);

/*
============================
Public Function Implementations
============================
*/

WhistRenderer* init_renderer(WhistFrontend* frontend, int initial_width, int initial_height) {
    WhistRenderer* whist_renderer = (WhistRenderer*)safe_malloc(sizeof(WhistRenderer));
    memset(whist_renderer, 0, sizeof(WhistRenderer));

    // Initialize audio and video systems
    whist_renderer->audio_context = init_audio(frontend);
    whist_renderer->video_context = init_video(frontend, initial_width, initial_height);

    if (SINGLE_THREAD_MODEL) {
        // These mutex/sem/timer pass work to the renderer thread when necessary
        start_timer(&whist_renderer->last_try_render_timer);
        whist_renderer->renderer_mutex = whist_create_mutex();
        whist_renderer->renderer_semaphore = whist_create_semaphore(0);
        // Used to log renderer thread usage
        whist_renderer->using_renderer_thread = false;
        whist_renderer->render_is_on_renderer_thread = false;

        // We use the renderer thread to do work,
        // If someone else hasn't called try_render recently
        whist_renderer->run_renderer_threads = true;
        whist_renderer->renderer_thread =
            whist_create_thread(multithreaded_renderer, "multithreaded_renderer", whist_renderer);
    } else {
        // Create sems
        whist_renderer->has_video_rendered_yet = false;
        whist_renderer->video_semaphore = whist_create_semaphore(0);
        whist_renderer->audio_semaphore = whist_create_semaphore(0);

        // Mark threads as running,
        whist_renderer->run_renderer_threads = true;
        whist_renderer->video_thread = whist_create_thread(
            multithreaded_video_renderer, "multithreaded_video_renderer", whist_renderer);
        whist_renderer->audio_thread = whist_create_thread(
            multithreaded_audio_renderer, "multithreaded_audio_renderer", whist_renderer);
    }

    // Return the struct
    return whist_renderer;
}

bool renderer_wants_frame(WhistRenderer* renderer, WhistPacketType packet_type,
                          int num_buffered_frames) {
    switch (packet_type) {
        case PACKET_VIDEO: {
            return video_ready_for_frame(renderer->video_context);
        }
        case PACKET_AUDIO: {
            return audio_ready_for_frame(renderer->audio_context, num_buffered_frames);
        }
        default: {
            LOG_FATAL("Unknown packet type! %d", (int)packet_type);
        }
    }
}

void renderer_receive_frame(WhistRenderer* whist_renderer, WhistPacketType packet_type,
                            void* frame) {
    WhistTimer statistics_timer;

    // Pass the receive packet into the video or audio context
    switch (packet_type) {
        case PACKET_VIDEO: {
            TIME_RUN(receive_video(whist_renderer->video_context, (VideoFrame*)frame),
                     VIDEO_RECEIVE_TIME, statistics_timer);
            if (!SINGLE_THREAD_MODEL) {
                whist_post_semaphore(whist_renderer->video_semaphore);
            }
            break;
        }
        case PACKET_AUDIO: {
            TIME_RUN(receive_audio(whist_renderer->audio_context, (AudioFrame*)frame),
                     AUDIO_RECEIVE_TIME, statistics_timer);
            if (!SINGLE_THREAD_MODEL) {
                whist_post_semaphore(whist_renderer->audio_semaphore);
            }
            break;
        }
        default: {
            LOG_FATAL("Unknown packet type! %d", (int)packet_type);
        }
    }
}

void renderer_update(WhistRenderer* whist_renderer) {
    if (!SINGLE_THREAD_MODEL) {
        return;
    }
    // If it's been 2 ms since the last time someone else called try_render,
    // let's ping our renderer thread to do the work instead

    // IF try_lock confirms we aren't rendering right now... [While also protecting
    // `last_try_render_timer`]
    if (whist_try_lock_mutex(whist_renderer->renderer_mutex) == 0) {
        // AND the timer confirms we haven't rendered recently... [Last ~2ms]
        double last_try_render_time_ms =
            get_timer(&whist_renderer->last_try_render_timer) * MS_IN_SECOND;
        if (last_try_render_time_ms > 2.0) {
            // Then..., we hit the semaphore so the renderer thread can do the rendering
            whist_post_semaphore(whist_renderer->renderer_semaphore);
        }

        // Unlock the mutex
        whist_unlock_mutex(whist_renderer->renderer_mutex);
    }
}

void renderer_try_render(WhistRenderer* whist_renderer) {
    if (!SINGLE_THREAD_MODEL) {
        return;
    }
    // Use a mutex to prevent multiple threads from rendering at once
    whist_lock_mutex(whist_renderer->renderer_mutex);

    if (LOG_RENDERER_THREAD_USAGE) {
        // Log render thread usage
        if (!whist_renderer->using_renderer_thread &&
            whist_renderer->render_is_on_renderer_thread) {
            LOG_INFO(
                "try_render has not been called externally recently, "
                "so defaulting to renderer thread usage now!");
            whist_renderer->using_renderer_thread = true;
        }
        if (whist_renderer->using_renderer_thread &&
            !whist_renderer->render_is_on_renderer_thread) {
            LOG_INFO(
                "try_render has been called from somewhere else! "
                "renderer thread will no longer be used now.");
            whist_renderer->using_renderer_thread = false;
        }
    }

    if (get_debug_console_override_values()->simulate_freeze) {
        whist_sleep(5);
    }

    // If the audio device is pending an update,
    // refresh the audio device
    if (pending_audio_device_update()) {
        refresh_audio_device(whist_renderer->audio_context);
    }

    // Render out any pending audio or video
    render_video(whist_renderer->video_context);
    if (has_video_rendered_yet(whist_renderer->video_context)) {
        // Only render audio, if the video has rendered something
        // This is because it feels weird when audio is played to the loading screen
        render_audio(whist_renderer->audio_context);
    }

    // Mark as recently rendered, and unlock
    start_timer(&whist_renderer->last_try_render_timer);
    whist_unlock_mutex(whist_renderer->renderer_mutex);
}

void destroy_renderer(WhistRenderer* whist_renderer) {
    // Wait to close the renderer thread
    whist_renderer->run_renderer_threads = false;
    if (SINGLE_THREAD_MODEL) {
        whist_post_semaphore(whist_renderer->renderer_semaphore);
        whist_wait_thread(whist_renderer->renderer_thread, NULL);
    } else {
        whist_post_semaphore(whist_renderer->video_semaphore);
        whist_post_semaphore(whist_renderer->audio_semaphore);
        whist_wait_thread(whist_renderer->video_thread, NULL);
        whist_wait_thread(whist_renderer->audio_thread, NULL);
        whist_destroy_semaphore(whist_renderer->video_semaphore);
        whist_destroy_semaphore(whist_renderer->audio_semaphore);
    }

    // Destroy the audio/video context
    destroy_audio(whist_renderer->audio_context);
    destroy_video(whist_renderer->video_context);

    // Free the whist renderer struct
    if (SINGLE_THREAD_MODEL) {
        whist_destroy_semaphore(whist_renderer->renderer_semaphore);
        whist_destroy_mutex(whist_renderer->renderer_mutex);
    }
    free(whist_renderer);
}

/*
============================
Private Function Implementations
============================
*/

int32_t multithreaded_renderer(void* opaque) {
    WhistRenderer* whist_renderer = (WhistRenderer*)opaque;

    while (true) {
        // Wait until we're told to render on this thread
        whist_wait_semaphore(whist_renderer->renderer_semaphore);

        // If this thread should no longer exist, exit accordingly
        if (!whist_renderer->run_renderer_threads) {
            break;
        }

        // Otherwise, try to render
        whist_renderer->render_is_on_renderer_thread = true;
        renderer_try_render(whist_renderer);
        whist_renderer->render_is_on_renderer_thread = false;
    }
    return 0;
}

int32_t multithreaded_video_renderer(void* opaque) {
    WhistRenderer* whist_renderer = (WhistRenderer*)opaque;

    // Whether or not video is still waiting to render
    bool pending_video = false;

    while (true) {
        // Wait until we're told to render on this thread
        if (pending_video) {
            whist_wait_timeout_semaphore(whist_renderer->video_semaphore, 1);
        } else {
            whist_wait_semaphore(whist_renderer->video_semaphore);
        }

        // If this thread should no longer exist, exit accordingly
        if (!whist_renderer->run_renderer_threads) {
            break;
        }

        // Otherwise, try to render, but note that 1 means the renderer is still pending
        // TODO: Make render_video internally semaphore on render, so we don't have to check
        if (render_video(whist_renderer->video_context) == 1) {
            pending_video = true;
        } else {
            pending_video = false;
            if (has_video_rendered_yet(whist_renderer->video_context)) {
                if (!whist_renderer->has_video_rendered_yet) {
                    // Notify audio that video has rendered
                    whist_renderer->has_video_rendered_yet = true;
                    whist_post_semaphore(whist_renderer->audio_semaphore);
                }
            }
        }
    }

    return 0;
}

int32_t multithreaded_audio_renderer(void* opaque) {
    WhistRenderer* whist_renderer = (WhistRenderer*)opaque;

    while (true) {
        // Wait until we're told to render on this thread

        if(whist_semaphore_value(whist_renderer->audio_semaphore) >0 )
        {
            whist_wait_semaphore(whist_renderer->audio_semaphore);

            // If this thread should no longer exist, exit accordingly
            if (!whist_renderer->run_renderer_threads) {
                break;
            }

            // Don't render if video hasn't rendered yet
            // This is because it feels weird when audio is played to the loading screen
            if (!whist_renderer->has_video_rendered_yet) {
                continue;
            }

            // Refresh the audio device, if a new audio device update is pending
            if (pending_audio_device_update()) {
                refresh_audio_device(whist_renderer->audio_context);
            }

            // Render the audio
            render_audio(whist_renderer->audio_context);
        }
        else
        {
            // dirty code just for POC
            render_audio_empty_loop(whist_renderer->audio_context);
            whist_sleep(2);
        }
    }

    return 0;
}
