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

struct WhistRenderer {
    VideoContext* video_context;
    AudioContext* audio_context;

    // for quit the render threads
    bool run_renderer_threads;

    // one thread for video and one thread for audio
    WhistThread video_thread;
    WhistThread audio_thread;

    // to implement the feature of not playing audio until video is played
    bool has_video_rendered_yet;

    // for blocking and waking video and audio thread
    WhistSemaphore video_semaphore;
    WhistSemaphore audio_semaphore;
};

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

void renderer_receive_frame(WhistRenderer* whist_renderer, WhistPacketType packet_type, void* frame,
                            size_t size) {
    WhistTimer statistics_timer;

    // Pass the receive packet into the video or audio context
    switch (packet_type) {
        case PACKET_VIDEO: {
            TIME_RUN(receive_video(whist_renderer->video_context, frame, size), VIDEO_RECEIVE_TIME,
                     statistics_timer);
            whist_post_semaphore(whist_renderer->video_semaphore);
            break;
        }
        case PACKET_AUDIO: {
            TIME_RUN(receive_audio(whist_renderer->audio_context, (AudioFrame*)frame),
                     AUDIO_RECEIVE_TIME, statistics_timer);
            whist_post_semaphore(whist_renderer->audio_semaphore);
            break;
        }
        default: {
            LOG_FATAL("Unknown packet type! %d", (int)packet_type);
        }
    }
}

void destroy_renderer(WhistRenderer* whist_renderer) {
    // Wait to close the renderer thread
    whist_renderer->run_renderer_threads = false;

    whist_post_semaphore(whist_renderer->video_semaphore);
    whist_post_semaphore(whist_renderer->audio_semaphore);
    whist_wait_thread(whist_renderer->video_thread, NULL);
    whist_wait_thread(whist_renderer->audio_thread, NULL);
    whist_destroy_semaphore(whist_renderer->video_semaphore);
    whist_destroy_semaphore(whist_renderer->audio_semaphore);

    // Destroy the audio/video context
    destroy_audio(whist_renderer->audio_context);
    destroy_video(whist_renderer->video_context);

    free(whist_renderer);
}

/*
============================
Private Function Implementations
============================
*/

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

    return 0;
}
