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
#include "client_statistic.h"
#include <whist/logging/log_statistic.h>
#include <whist/network/network.h>
#include <whist/network/ringbuffer.h>
#include <whist/core/whist_frame.h>

/*
============================
Defines
============================
*/

// Verbose audio logs
#define LOG_AUDIO false

// Used to prevent audio from playing before video has played
extern bool has_video_rendered_yet;

#define SAMPLES_PER_FRAME 480
#define BYTES_PER_SAMPLE 4
#define NUM_CHANNELS 2
#define DECODED_BYTES_PER_FRAME (SAMPLES_PER_FRAME * BYTES_PER_SAMPLE * NUM_CHANNELS)

// Audio frames in the audio ringbuffer
#define MAX_NUM_AUDIO_FRAMES 8

// system audio queue + our buffer limits, in number of frames and decompressed bytes
#define AUDIO_QUEUE_LOWER_LIMIT_FRAMES 1
#define AUDIO_QUEUE_UPPER_LIMIT_FRAMES MAX_NUM_AUDIO_FRAMES
#define TARGET_AUDIO_QUEUE_LIMIT_FRAMES 5
#define AUDIO_QUEUE_LOWER_LIMIT (AUDIO_QUEUE_LOWER_LIMIT_FRAMES * DECODED_BYTES_PER_FRAME)
#define AUDIO_QUEUE_UPPER_LIMIT (AUDIO_QUEUE_UPPER_LIMIT_FRAMES * DECODED_BYTES_PER_FRAME)
#define TARGET_AUDIO_QUEUE_LIMIT (TARGET_AUDIO_QUEUE_LIMIT_FRAMES * DECODED_BYTES_PER_FRAME)

#define SDL_AUDIO_BUFFER_SIZE SAMPLES_PER_FRAME

// Maximum valid audio frequency
#define MAX_FREQ 128000

/*
============================
Custom Types
============================
*/

struct AudioContext {
    // Currently set sample rate of the audio device/decoder
    int audio_frequency;

    // True if the audio device is pending a refresh
    bool pending_refresh;

    RingBuffer* ring_buffer;
    SDL_AudioDeviceID dev;
    AudioDecoder* audio_decoder;

    // Audio render context

    // holds the current audio frame pending to render
    AudioFrame* render_context;
    // true if and only if the audio packet in render_context should be played
    bool pending_render_context;

    // The last ID we've processed for rendering
    int last_played_id;

    // Overflow/underflow state
    bool is_flushing_audio;
    bool is_buffering_audio;
};

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Initialize the SDL audio device,
 *                                 and the ffmpeg audio decoder.
 *                                 They must not already exist.
 *
 * @param audio_context            The audio context to use
 * 
 * @note                           On failure, audio_context->dev will remain 0,
 *                                 and audio_context->audio_decoder will remain NULL.
 */
static void init_audio_device(AudioContext* audio_context);

/**
 * @brief                          Destroy the SDL audio device,
 *                                 and the ffmpeg audio decoder,
 *                                 if they exist.
 *
 * @param audio_context            The audio context to use
 * 
 * @note                           If SDL audio device / ffmpeg have not been initialized,
 *                                 this function will simply do nothing
 */
static void destroy_audio_device(AudioContext* audio_context);

/**
 * @brief                          Checks if the system queue is overflowing 
 *
 * @returns                        If True, we can receive more audio frames,
 *                                 but they will just be discarded.
 */
static bool is_overflowing_audio(AudioContext* audio_context);

/**
 * @brief                          Checks if we don't have enough audio in queue / ringbuffer
 *
 * @returns                        If True, we should not drain from the ringbuffer
 *                                 until is_underflowing_audio returns false.
 */
static bool is_underflowing_audio(AudioContext* audio_context, int num_frame_buffered);

/*
============================
Public Function Implementations
============================
*/

AudioContext* init_audio() {
    LOG_INFO("Initializing audio system");

    // Allocate the audio context
    AudioContext* audio_context = safe_malloc(sizeof(*audio_context));
    memset(audio_context, 0, sizeof(*audio_context));

    // Initialize everything
    audio_context->audio_frequency = -1;
    audio_context->pending_refresh = true;
    audio_context->pending_render_context = false;
    audio_context->last_played_id = -1;
    audio_context->dev = 0;
    audio_context->audio_decoder = NULL;
    audio_context->is_flushing_audio = false;
    audio_context->is_buffering_audio = false;

    // Return the audio context
    return audio_context;
}

void destroy_audio(AudioContext* audio_context) {
    LOG_INFO("Destroying audio system");

    // Destroy the audio device
    destroy_audio_device(audio_context);

    // Free the audio struct
    destroy_ring_buffer(audio_context->ring_buffer);
    free(audio_context);
}

void refresh_audio_device(AudioContext* audio_context) {
    // Mark the audio device as pending a refresh
    audio_context->pending_refresh = true;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
void receive_audio(AudioContext* audio_context, WhistPacket* packet) {
    // ===========================
    // BEGIN NEW CODE
    // ===========================
    if (is_overflowing_audio(audio_context)) {
        // if we're overflowing, discard the packet but pretend we played it
        audio_context->last_played_id = packet->id;
        LOG_WARNING("Audio queue full, skipping ID %d", audio_context->last_played_id);
        return;
    }
    // otherwise, push the audio frame to the render context
    if (!audio_context->pending_render_context) {
        // send a frame to the renderer and set audio_context->pending_render_context to true to signal readiness
        if (audio_context->last_played_id != -1) {
            // give data pointer to the audio context
            audio_context->render_context = packet->data;
            // increment last_rendered_id
            LOG_DEBUG("received packet with ID %d, audio last played %d", packet->id, audio_context->last_played_id);
            audio_context->last_played_id = packet->id;
            // signal to the renderer that we're ready
            audio_context->pending_render_context = true;
        }
    } else {
        LOG_ERROR("We tried to send the audio context a frame when it wasn't ready!");
    }
    // ===========================
    // END NEW CODE - DELETE CODE AFTER THIS
    // ===========================
}

void render_audio(AudioContext* audio_context) {
    if (audio_context->pending_render_context) {
        // Only do work, if the audio frequency is valid
        FATAL_ASSERT(audio_context->render_context != NULL);
        AudioFrame* audio_frame = (AudioFrame*)audio_context->render_context;

        // Mark as pending refresh when the audio frequency is being updated
        if (audio_context->audio_frequency != audio_frame->audio_frequency) {
            LOG_INFO("Updating audio frequency to %d!", audio_frame->audio_frequency);
            audio_context->audio_frequency = audio_frame->audio_frequency;
            audio_context->pending_refresh = true;
        }

        // Note that because sdl_event_handler.c also sets pending_refresh to true when changing
        // audio devices, there is a minor race condition that can lead to reinit_audio_device()
        // being called more times than expected.
        if (audio_context->pending_refresh) {
            // Consume the pending audio refresh
            audio_context->pending_refresh = false;
            // Update the audio device
            destroy_audio_device(audio_context);
            init_audio_device(audio_context);
        }

        // If we have a valid audio device to render with...
        if (audio_context->dev != 0) {
            // Send the encoded frame to the decoder
            if (audio_decoder_send_packets(audio_context->audio_decoder, audio_frame->data,
                                        audio_frame->data_length) < 0) {
                LOG_FATAL("Failed to send packets to decoder!");
            }

            // While there are frames to decode...
            while (true) {
                // Decode the frame
                int res = audio_decoder_get_frame(audio_context->audio_decoder);
                if (res == 0) {
                    // Buffer to hold the decoded data
                    static uint8_t decoded_data[MAX_AUDIO_FRAME_SIZE];
                    // Get the decoded data
                    audio_decoder_packet_readout(audio_context->audio_decoder, decoded_data);

                    // Queue the decoded_data into the audio device
                    res =
                        SDL_QueueAudio(audio_context->dev, decoded_data,
                                    audio_decoder_get_frame_data_size(audio_context->audio_decoder));

                    if (res < 0) {
                        LOG_WARNING("Could not play audio!");
                    }
                } else {
                    break;
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

static void init_audio_device(AudioContext* audio_context) {
    LOG_INFO("Initializing audio device");

    // Verify that the device doesn't already exist
    FATAL_ASSERT(audio_context->dev == 0);
    FATAL_ASSERT(audio_context->audio_decoder == NULL);

    // Initialize the SDL audio device
    SDL_AudioSpec wanted_spec = {0}, audio_spec = {0};

    SDL_zero(wanted_spec);
    SDL_zero(audio_spec);
    wanted_spec.channels = 2;
    wanted_spec.freq = audio_context->audio_frequency;
    wanted_spec.format = AUDIO_F32SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;

    audio_context->dev =
        SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (audio_context->dev == 0) {
        LOG_ERROR("Failed to open audio: %s", SDL_GetError());
        return;
    }

    if (wanted_spec.freq != audio_spec.freq) {
        LOG_ERROR("Got Frequency %d, But Wanted Frequency %d...", audio_spec.freq,
                  wanted_spec.freq);
    } else {
        LOG_INFO("Using Audio Freqency: %d", audio_spec.freq);
    }

    // Unpause the audio device, to allow playing
    SDL_PauseAudioDevice(audio_context->dev, 0);

    // Initialize the decoder
    audio_context->audio_decoder = create_audio_decoder(audio_context->audio_frequency);
}

static void destroy_audio_device(AudioContext* audio_context) {
    // Destroy the SDL audio device, if any exists
    if (audio_context->dev != 0) {
        SDL_CloseAudioDevice(audio_context->dev);
        audio_context->dev = 0;
    }

    // Destroy the audio decoder, if any exists
    if (audio_context->audio_decoder != NULL) {
        destroy_audio_decoder(audio_context->audio_decoder);
        audio_context->audio_decoder = NULL;
    }
}

int safe_get_audio_queue(AudioContext* audio_context) {
    int audio_device_queue = 0;
    if (audio_context->dev) {
        // If we have a device, get the queue size
        audio_device_queue = (int)SDL_GetQueuedAudioSize(audio_context->dev);
    }
#if LOG_AUDIO
    LOG_DEBUG("Audio Queue: %d", audio_device_queue);
#endif
    return audio_device_queue;
}

bool is_overflowing_audio(AudioContext* audio_context) {
    int audio_device_queue = safe_get_audio_queue(audio_context);

    // Check if we're overflowing the audio buffer
    if (!audio_context->is_flushing_audio && audio_device_queue > AUDIO_QUEUE_UPPER_LIMIT) {
        // Yes, we are! We should flush the audio buffer
        LOG_INFO("Audio queue has overflowed %d!", AUDIO_QUEUE_UPPER_LIMIT);
        audio_context->is_flushing_audio = true;
    } else if (audio_context->is_flushing_audio && audio_device_queue <= TARGET_AUDIO_QUEUE_LIMIT) {
        // Okay, we're done flushing the audio buffer
        audio_context->is_flushing_audio = false;
    }

    return audio_context->is_flushing_audio;
}

bool is_underflowing_audio(AudioContext* audio_context, int num_frames_buffered) {
    int buffered_bytes = safe_get_audio_queue(audio_context) + num_frames_buffered * DECODED_BYTES_PER_FRAME;

    // Check if we're underflowing the audio buffer
    if (!audio_context->is_buffering_audio && buffered_bytes < AUDIO_QUEUE_LOWER_LIMIT) {
        // Yes, we are! We should buffer for some more audio then
        LOG_INFO("Audio Queue too low: only %d bytes left!", buffered_bytes);
        audio_context->is_buffering_audio = true;
    } else if (audio_context->is_buffering_audio && buffered_bytes >= TARGET_AUDIO_QUEUE_LIMIT) {
        // Okay, we're done buffering audio
        LOG_INFO("Done catching up! Now have %d bytes in queue", buffered_bytes);
        audio_context->is_buffering_audio = false;
    }

    return audio_context->is_buffering_audio;
}

bool audio_ready_for_frame(AudioContext* audio_context, int num_frames_buffered) {
    return !audio_context->pending_render_context && !is_underflowing_audio(audio_context, num_frames_buffered);
}
