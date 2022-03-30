/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file audio.c
 * @brief This file contains all code that interacts directly with processing
 *        audio packets on the client.
============================
Usage
============================

initAudio() must be called first before receiving any audio packets.
updateAudio() gets called immediately after to update the client to the server's
audio format.
*/

/*
============================
Includes
============================
*/

#include "audio.h"
#include "network.h"
#include <whist/logging/log_statistic.h>
#include <whist/network/network.h>
#include <whist/network/ringbuffer.h>
#include <whist/core/whist_frame.h>
#include <whist/debug/protocol_analyzer.h>
#include <whist/debug/debug_console.h>

/*
============================
Defines
============================
*/

// TODO: Automatically deduce this from (ms_per_frame / MS_IN_SECOND) * audio_frequency
// TODO: Add ms_per_frame to AudioFrame*
#define SAMPLES_PER_FRAME 480
#define BYTES_PER_SAMPLE 4
#define NUM_CHANNELS 2
#define DECODED_BYTES_PER_FRAME (SAMPLES_PER_FRAME * BYTES_PER_SAMPLE * NUM_CHANNELS)

// The size of the frontend audio queue that we're aiming for
// In frames
#define AUDIO_QUEUE_TARGET_SIZE 8
// The total size of audio-queue and userspace buffer to overflow at
// In frames
#define AUDIO_BUFFER_OVERFLOW_SIZE 20

// The time per size sample
#define AUDIO_SAMPLE_FREQUENCY_MS 100
// The number of samples we use for average estimation
#define AUDIO_NUM_SAMPLES 10
// Acceptable size discrepancy that the average sample size could have
// [AUDIO_QUEUE_TARGET_SIZE-AUDIO_ACCEPTABLE_DELTA, AUDIO_QUEUE_TARGET_SIZE+AUDIO_ACCEPTABLE_DELTA]
#define AUDIO_ACCEPTABLE_DELTA 1.2

/*
============================
Custom Types
============================
*/

typedef enum {
    InitialBuffering,
    Normal,
} AudioState;

typedef enum {
    None,
    DropFrame,
    DupFrame,
} AdjustCommand;

typedef struct {
    AdjustCommand adjust_command;
    AudioFrame* audio_frame;
} AudioRenderContext;

struct AudioContext {
    // Currently set sample rate of the audio device/decoder
    int audio_frequency;

    // True if the audio device is pending a refresh
    bool pending_refresh;

    AudioDecoder* audio_decoder;

    WhistFrontend* target_frontend;

    // Audio render context

    // holds the current audio frame pending to render
    AudioRenderContext render_context;
    // true if and only if the audio packet in render_context should be played
    bool pending_render_context;

    // Audio state
    AudioState audio_state;

    // Sample sizes for average size tracking
    // Samples are measured in frames (Not necessarily whole numbers)
    WhistTimer size_sample_timer;
    int sample_index;
    double samples[AUDIO_NUM_SAMPLES];
    // The adjust as recommended by size sample analysis
    // This will get set to None once it's acted upon
    AdjustCommand adjust_command;
};

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Initializes the audio device and decoder
 *                                 of the given audio context
 *
 * @param audio_context            The audio context to initialize
 */
static void init_audio_player(AudioContext* audio_context);

/**
 * @brief                          Destroy frontend audio device
 *                                 and the ffmpeg audio decoder,
 *                                 if they exist.
 *
 * @param audio_context            The audio context to use
 *
 * @note                           If frontend audio device / ffmpeg have not been initialized,
 *                                 this function will simply do nothing
 */
static void destroy_audio_player(AudioContext* audio_context);

/**
 * @brief                          Gets the audio device queue size of the audio context
 *
 * @param audio_context            The audio context to use
 *
 * @returns                        The size of the audio device queue
 *
 * @note                           This function is thread-safe
 */
static size_t safe_get_audio_queue(AudioContext* audio_context);

/*
============================
Public Function Implementations
============================
*/

AudioContext* init_audio(WhistFrontend* frontend) {
    LOG_INFO("Initializing audio system targeting frontend %u", whist_frontend_get_id(frontend));

    // Allocate the audio context
    AudioContext* audio_context = safe_malloc(sizeof(*audio_context));
    memset(audio_context, 0, sizeof(*audio_context));

    // Initialize everything
    audio_context->audio_frequency = AUDIO_FREQUENCY;
    audio_context->pending_refresh = false;
    audio_context->pending_render_context = false;
    audio_context->target_frontend = frontend;
    audio_context->audio_decoder = NULL;
    audio_context->audio_state = InitialBuffering;
    init_audio_player(audio_context);

    // Init size samples
    start_timer(&audio_context->size_sample_timer);
    audio_context->sample_index = 0;
    for (int i = 0; i < AUDIO_NUM_SAMPLES; i++) {
        audio_context->samples[i] = AUDIO_QUEUE_TARGET_SIZE;
    }

    // Return the audio context
    return audio_context;
}

void destroy_audio(AudioContext* audio_context) {
    LOG_INFO("Destroying audio system targeting frontend %u",
             whist_frontend_get_id(audio_context->target_frontend));

    // Destroy the audio device
    destroy_audio_player(audio_context);

    // Free the audio struct
    free(audio_context);
}

void refresh_audio_device(AudioContext* audio_context) {
    // Mark the audio device as pending a refresh
    audio_context->pending_refresh = true;
}

bool audio_ready_for_frame(AudioContext* audio_context, int num_frames_buffered) {
    if (get_debug_console_override_values()->verbose_log_audio) {
        LOG_INFO("[audio]pending_render_context= %d", (int)audio_context->pending_render_context);
    }

    // Get device size and total buffer size
    int audio_device_size = (int)safe_get_audio_queue(audio_context);
    int audio_size = audio_device_size + num_frames_buffered * DECODED_BYTES_PER_FRAME;

    // Every frequency ms, record an audio sample
    if (get_timer(&audio_context->size_sample_timer) * MS_IN_SECOND > AUDIO_SAMPLE_FREQUENCY_MS) {
        // Record the sample and reset the timer
        audio_context->samples[audio_context->sample_index] = audio_size / DECODED_BYTES_PER_FRAME;
        audio_context->sample_index++;
        start_timer(&audio_context->size_sample_timer);
        // If we've sampled numsamples time, act on the average size
        if (audio_context->sample_index == AUDIO_NUM_SAMPLES) {
            audio_context->sample_index = 0;
            // Calculate the new average, in fractional frames
            double avg_sample = 0.0;
            for (int i = 0; i < AUDIO_NUM_SAMPLES; i++) {
                avg_sample += audio_context->samples[i];
            }
            avg_sample /= AUDIO_NUM_SAMPLES;
            // Check for size-target discrepancy
            audio_context->adjust_command = None;
            if (avg_sample < AUDIO_QUEUE_TARGET_SIZE - AUDIO_ACCEPTABLE_DELTA) {
                audio_context->adjust_command = DupFrame;
            }
            if (avg_sample > AUDIO_QUEUE_TARGET_SIZE + AUDIO_ACCEPTABLE_DELTA) {
                audio_context->adjust_command = DropFrame;
            }
        }
    }

    // Consume a new frame, if the renderer has room to queue 8 frames, or if an overflow happened,
    // or if we need to adjust
    int num_frames_to_render = audio_context->adjust_command == DupFrame ? 2 : 1;
    bool wants_new_frame = !audio_context->pending_render_context &&
                           audio_device_size <= (AUDIO_QUEUE_TARGET_SIZE - num_frames_to_render) *
                                                    DECODED_BYTES_PER_FRAME;
    bool frames_overflowing = audio_size > AUDIO_BUFFER_OVERFLOW_SIZE * DECODED_BYTES_PER_FRAME;
    return wants_new_frame || frames_overflowing || audio_context->adjust_command == DropFrame;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
void receive_audio(AudioContext* audio_context, AudioFrame* audio_frame) {
    // otherwise, push the audio frame to the render context
    if (!audio_context->pending_render_context) {
        // If we have space, render it out. Otherwise, drop it because we're overflowing.
        int num_frames_to_render = audio_context->adjust_command == DupFrame ? 2 : 1;
        bool has_space = (int)safe_get_audio_queue(audio_context) <=
                         (AUDIO_QUEUE_TARGET_SIZE - num_frames_to_render) * DECODED_BYTES_PER_FRAME;
        bool should_drop = !has_space || audio_context->adjust_command == DropFrame;
        if (!should_drop) {
            // Mark out the audio frame to the render context
            audio_context->render_context.adjust_command = audio_context->adjust_command;
            audio_context->render_context.audio_frame = audio_frame;

            // LOG_DEBUG("received packet with ID %d, audio last played %d", packet->id,
            // audio_context->last_played_id); signal to the renderer that we're ready

            whist_analyzer_record_pending_rendering(PACKET_AUDIO);
            audio_context->pending_render_context = true;
        }
        audio_context->adjust_command = None;
    } else {
        LOG_ERROR("We tried to send the audio context a frame when it wasn't ready!");
    }
}

void render_audio(AudioContext* audio_context) {
    if (audio_context->pending_render_context) {
        // Only do work, if the audio frequency is valid
        FATAL_ASSERT(audio_context->render_context.audio_frame != NULL);
        AudioFrame* audio_frame = (AudioFrame*)audio_context->render_context.audio_frame;

        // Mark as pending refresh when the audio frequency is being updated
        if (audio_context->audio_frequency != audio_frame->audio_frequency) {
            LOG_INFO("Updating audio frequency to %d!", audio_frame->audio_frequency);
            audio_context->audio_frequency = audio_frame->audio_frequency;
            audio_context->pending_refresh = true;
        }

        // Note that because sdl_event_handler.c also sets pending_refresh to true when changing
        // audio devices, there is a minor race condition that can lead to reinit_audio_player()
        // being called more times than expected.
        if (audio_context->pending_refresh) {
            // Consume the pending audio refresh
            audio_context->pending_refresh = false;
            // Update the audio device
            destroy_audio_player(audio_context);
            init_audio_player(audio_context);
        }

        // If we have a valid audio device to render with...
        if (whist_frontend_audio_is_open(audio_context->target_frontend)) {
            whist_analyzer_record_decode_audio();
            // Send the encoded frame to the decoder
            if (audio_decoder_send_packets(audio_context->audio_decoder, audio_frame->data,
                                           audio_frame->data_length) < 0) {
                LOG_FATAL("Failed to send packets to decoder!");
            }

            // While there are frames to decode...
            while (audio_decoder_get_frame(audio_context->audio_decoder) == 0) {
                // Buffer to hold the decoded data
                uint8_t decoded_data[MAX_AUDIO_FRAME_SIZE];
                size_t decoded_data_size;

                // Get the decoded data
                // TODO: Combine these functions into one by returning the decoded data size
                // in the readout function.
                audio_decoder_packet_readout(audio_context->audio_decoder, decoded_data);
                decoded_data_size = audio_decoder_get_frame_data_size(audio_context->audio_decoder);

                // Queue the decoded_data into the frontend audio player
                if (whist_frontend_queue_audio(audio_context->target_frontend, decoded_data,
                                               decoded_data_size) < 0) {
                    LOG_WARNING("Could not play audio!");
                }
                // If we should dup the frame, queue it out again
                if (audio_context->render_context.adjust_command == DupFrame) {
                    if (whist_frontend_queue_audio(audio_context->target_frontend, decoded_data,
                                                   decoded_data_size) < 0) {
                        LOG_WARNING("Could not play audio!");
                    }
                }
            }
        }

        // No longer rendering audio
        audio_context->pending_render_context = false;
    }
}

/*
============================
Private Function Implementations
============================
*/

static void init_audio_player(AudioContext* audio_context) {
    // Initialize the audio device for the frequency and 2 channels
    whist_frontend_open_audio(audio_context->target_frontend, audio_context->audio_frequency, 2);
    // NOTE: Pause audio here

    // Verify that the decoder doesn't already exist
    FATAL_ASSERT(audio_context->audio_decoder == NULL);

    // Initialize the decoder
    audio_context->audio_decoder = create_audio_decoder(audio_context->audio_frequency);
}

static void destroy_audio_player(AudioContext* audio_context) {
    // Destroy the SDL audio device, if any exists
    whist_frontend_close_audio(audio_context->target_frontend);

    // Destroy the audio decoder, if any exists
    if (audio_context->audio_decoder != NULL) {
        destroy_audio_decoder(audio_context->audio_decoder);
        audio_context->audio_decoder = NULL;
    }
}

static size_t safe_get_audio_queue(AudioContext* audio_context) {
    size_t audio_queue = 0;
    if (whist_frontend_audio_is_open(audio_context->target_frontend)) {
        audio_queue = whist_frontend_get_audio_buffer_size(audio_context->target_frontend);
    }
    if (LOG_AUDIO) {
        LOG_DEBUG("Audio Queue: %zu", audio_queue);
    }
    return audio_queue;
}
