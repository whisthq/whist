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
#include "client_statistic.h"
#include "sdl_event_handler.h"

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Renders audio/video on a separate thread
 */
int32_t multithreaded_renderer(void* opaque);

/*
============================
Public Function Implementations
============================
*/

WhistRenderer* init_renderer() {
    WhistRenderer* whist_renderer = (WhistRenderer*)safe_malloc(sizeof(WhistRenderer));

    whist_renderer->video_context = init_video();
    whist_renderer->audio_context = init_audio();

    whist_renderer->run_renderer_thread = true;
    whist_renderer->renderer_thread =
        whist_create_thread(multithreaded_renderer, "multithreaded_renderer", whist_renderer);

    return whist_renderer;
}

void renderer_receive_packet(WhistRenderer* whist_renderer, WhistPacket* packet) {
    clock statistics_timer;

    // Pass the receive packet into the video or audio context
    switch (packet->type) {
        case PACKET_AUDIO: {
            TIME_RUN(receive_audio(whist_renderer->audio_context, packet), AUDIO_RECEIVE_TIME,
                     statistics_timer);
            break;
        }
        case PACKET_VIDEO: {
            TIME_RUN(receive_video(whist_renderer->video_context, packet), VIDEO_RECEIVE_TIME,
                     statistics_timer);
            break;
        }
        default: {
            LOG_FATAL("Unknown packet type! %d", packet->type);
        }
    }
}

void renderer_update(WhistRenderer* whist_renderer) {
    clock statistics_timer;

    // Update video/audio
    TIME_RUN(update_audio(whist_renderer->audio_context), AUDIO_UPDATE_TIME, statistics_timer);
    TIME_RUN(update_video(whist_renderer->video_context), VIDEO_UPDATE_TIME, statistics_timer);
}

void renderer_try_render(WhistRenderer* whist_renderer) {
    // If the audio device is pending an update,
    // refresh the audio device
    if (sdl_pending_audio_device_update()) {
        refresh_audio_device(whist_renderer->audio_context);
    }
    // Render out any pending audio or video
    render_audio(whist_renderer->audio_context);
    render_video(whist_renderer->video_context);
}

void destroy_renderer(WhistRenderer* whist_renderer) {
    // Wait to close the renderer thread
    whist_renderer->run_renderer_thread = false;
    whist_wait_thread(whist_renderer->renderer_thread, NULL);

    // Destroy the audio/video context
    destroy_audio(whist_renderer->audio_context);
    destroy_video(whist_renderer->video_context);

    // Free the whist renderer struct
    free(whist_renderer);
}

/*
============================
Private Function Implementations
============================
*/

int32_t multithreaded_renderer(void* opaque) {
    WhistRenderer* whist_renderer = (WhistRenderer*)opaque;

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while (whist_renderer->run_renderer_thread) {
        render_audio(whist_renderer->audio_context);
        render_video(whist_renderer->video_context);
        whist_usleep(0.25 * US_IN_MS);
    }

    return 0;
}
