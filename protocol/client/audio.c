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
    bool audio_flush_triggered;
};

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Initialize the SDL audio device,
 *                                 and the ffmpeg audio decoder
 *
 * @param audio_context            The audio context to use
 */
static void init_audio_device(AudioContext* audio_context);

/**
 * @brief                          Destroy the SDL audio device,
 *                                 and the ffmpeg audio decoder
 *
 * @param audio_context            The audio context to use
 */
static void destroy_audio_device(AudioContext* audio_context);

/**
 * @brief                          If the ringbuffer has accumulated too many
 *                                 frames, this will catch up to the most recent
 *                                 ID and invalidate the old ones.
 *
 * @param audio_context            The audio context to use
 */
static void catchup_audio(AudioContext* audio_context);

/**
 * @brief                          Checks whether or not we should be
 *                                 buffering audio right now. Note that we should
 *                                 buffer audio if the audio device's internal audio
 *                                 queue ever runs too low.
 *
 * @param audio_context            The audio context
 *
 * @returns                        If True, we should not push any render context,
 *                                 and we should instead let the ringbuffer accumulate.
 *                                 If False, we can push render contexts out to render.
 */
static bool is_buffering_audio(AudioContext* audio_context);

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
    audio_context->ring_buffer = init_ring_buffer(PACKET_AUDIO, MAX_NUM_AUDIO_FRAMES, nack_packet);
    audio_context->last_played_id = -1;
    audio_context->audio_flush_triggered = false;
    audio_context->dev = 0;
    audio_context->audio_decoder = NULL;

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
    int res = receive_packet(audio_context->ring_buffer, packet);
#if LOG_AUDIO
    LOG_DEBUG("Received packet with ID/Index %d/%d", packet->id, packet->index);
#endif
    if (res < 0) {
        LOG_ERROR("Ringbuffer packet error!");
    } else if (res > 0) {
        if (audio_context->ring_buffer->currently_rendering_id != -1) {
            log_double_statistic(AUDIO_FPS_SKIPPED, (double)res);
        }
        if (audio_context->last_played_id < packet->id && audio_context->last_played_id > 0) {
            audio_context->last_played_id = packet->id - 1;
            FrameData* frame_data =
                get_frame_at_id(audio_context->ring_buffer, audio_context->last_played_id);
            if (frame_data->id != -1) {
                reset_frame(audio_context->ring_buffer, frame_data);
            }
            LOG_INFO("Last played ID is now %d", audio_context->last_played_id);
        }
    }
}

void update_audio(AudioContext* audio_context) {
    if (audio_context->pending_render_context) {
        // If we're currently rendering an audio packet, don't update audio - the audio_context
        // struct is being used, so a race condition will occur if we call SDL_GetQueuedAudioSize at
        // the same time.
        return;
    }

    catchup_audio(audio_context);

    RingBuffer* ring_buffer = audio_context->ring_buffer;

    // Return if there's nothing to play or if last_played_id > max_id (eg ring buffer reset)
    if (audio_context->last_played_id == -1 ||
        audio_context->last_played_id > ring_buffer->max_id) {
        return;
    }

    // The ID of the packet we're trying to play next
    int next_to_play_id = audio_context->last_played_id + 1;

#if LOG_AUDIO
    // Log details of the current next-to-play packet
    FrameData* frame_data = get_frame_at_id(ring_buffer, next_to_play_id);
    if (frame_data->id != -1) {
        LOG_DEBUG("Audio next_to_play_id: %d, Ringbuffer ID: %d, Packets %d/%d", next_to_play_id,
                  frame_data->id,
                  frame_data->fec_packets_received + frame_data->original_packets_received,
                  frame_data->num_original_packets);
    }
#endif

    // Return if we are buffering audio to build up the queue.
    if (is_buffering_audio(audio_context)) {
        return;
    }

    // If the next-to-play packet is ready to render...
    if (is_ready_to_render(ring_buffer, next_to_play_id)) {
#if LOG_AUDIO
        LOG_INFO("Moving audio frame ID %d into render context", next_to_play_id);
#endif
        // Grab the next-to-play AudioFrame and put it in the render context
        audio_context->render_context =
            (AudioFrame*)set_rendering(ring_buffer, next_to_play_id)->frame_buffer;
        // Mark the render context as being pushed
        audio_context->pending_render_context = true;
        // Increment the last played ID since we've rendered next_to_play_id out
        audio_context->last_played_id = next_to_play_id;
    }
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
    if (audio_context->dev) {
        SDL_CloseAudioDevice(audio_context->dev);
        audio_context->dev = 0;
    }

    // Destroy the audio decoder, if any exists
    if (audio_context->audio_decoder) {
        destroy_audio_decoder(audio_context->audio_decoder);
        audio_context->audio_decoder = NULL;
    }
}

void catchup_audio(AudioContext* audio_context) {
    RingBuffer* ring_buffer = audio_context->ring_buffer;
    audio_context->last_played_id = audio_context->last_played_id;

    // If nothing has played yet, or if we've fallen far behind (because of a disconnect),
    // Then catch-up by setting last_played_id to max_id - 1
    if ((audio_context->last_played_id == -1 && ring_buffer->max_id > 0) ||
        (audio_context->last_played_id != -1 &&
         ring_buffer->max_id - audio_context->last_played_id > MAX_NUM_AUDIO_FRAMES)) {
#if LOG_AUDIO
        LOG_DEBUG("Catching up audio from ID %d to ID %d", audio_context->last_played_id,
                  ring_buffer->max_id - 1);
#endif
        if (audio_context->last_played_id != -1) {
            log_double_statistic(AUDIO_FPS_SKIPPED,
                                 (double)(ring_buffer->max_id - audio_context->last_played_id - 1));
        }
        audio_context->last_played_id = ring_buffer->max_id - 1;
    }

    // Wipe all of the stale frames, i.e. all IDs that are on or before the last played ID
    for (int i = 0; i < MAX_NUM_AUDIO_FRAMES; i++) {
        FrameData* frame_data = &ring_buffer->receiving_frames[i];
        if (frame_data->id <= audio_context->last_played_id && frame_data->id != -1) {
            reset_frame(ring_buffer, frame_data);
        }
    }
}

bool is_buffering_audio(AudioContext* audio_context) {
    // Get the size of the audio device queue,
    int audio_device_queue = 0;
    if (audio_context->dev) {
        // If we have a device, get the queue size
        audio_device_queue = (int)SDL_GetQueuedAudioSize(audio_context->dev);
    }
    // Otherwise, the queue size is 0

#if LOG_AUDIO
    LOG_DEBUG("Audio Queue: %d", audio_device_queue);
#endif

    // ========
    // Code for when the audio queue is underflowing
    // ========

    int max_id = audio_context->ring_buffer->max_id;
    int last_played_id = audio_context->last_played_id;

    static bool buffering_audio = false;
    int bytes_until_no_more_audio =
        (max_id - last_played_id) * DECODED_BYTES_PER_FRAME + audio_device_queue;

    // If the audio queue is under AUDIO_QUEUE_LOWER_LIMIT, we need to accumulate more in the buffer
    if (!buffering_audio && bytes_until_no_more_audio < AUDIO_QUEUE_LOWER_LIMIT) {
        LOG_INFO("Audio Queue too low: %d. max_id %d, last_played_id %d. Needs to catch up!",
                 bytes_until_no_more_audio, max_id, last_played_id);
        buffering_audio = true;
    }

    // Don't play anything until we have enough audio in the queue
    if (buffering_audio) {
        if (bytes_until_no_more_audio >= TARGET_AUDIO_QUEUE_LIMIT) {
            LOG_INFO("Done catching up! Audio Queue: %d, max_id %d, last_played_id %d",
                     bytes_until_no_more_audio, max_id, last_played_id);
            buffering_audio = false;
        }
    }

    // ========
    // Code for when the audio queue is overflowing
    // ========

    // If the audio queue surpasses AUDIO_QUEUE_UPPER_LIMIT,
    // we trigger an audio flush.
    // Once an audio flush is triggered,
    // We skip audio frames until the buffer runs down to below TARGET_AUDIO_QUEUE_LIMIT

    // Mark an audio flush as newly triggered, if we've overflowed the upper limit
    if (audio_device_queue > AUDIO_QUEUE_UPPER_LIMIT && !audio_context->audio_flush_triggered) {
        LOG_INFO("Audio queue has overflowed %d!", AUDIO_QUEUE_UPPER_LIMIT);
        audio_context->audio_flush_triggered = true;
    }

    // While an audio flush was triggered,
    if (audio_context->audio_flush_triggered) {
        // If the device queue is still above TARGET_AUDIO_QUEUE_LIMIT,
        if (audio_device_queue > TARGET_AUDIO_QUEUE_LIMIT) {
            // _and_ there's a frame trying to render => Skip that frame!
            int next_to_play_id = audio_context->last_played_id + 1;
            if (is_ready_to_render(audio_context->ring_buffer, next_to_play_id)) {
                // Skip the frame, by ignoring the FrameData* returned
                set_rendering(audio_context->ring_buffer, next_to_play_id);
                audio_context->last_played_id = next_to_play_id;

                // Log the skipped frame
                log_double_statistic(AUDIO_FPS_SKIPPED, 1.0);
                LOG_WARNING("Audio queue full, skipping ID %d (Queued: %d)", next_to_play_id,
                            audio_device_queue);
            }
        } else {
            // If the device queue isn't too large, stop flushing it.
            audio_context->audio_flush_triggered = false;
        }
    }

    // Return true if we're buffering audio or if we're flushing audio
    return buffering_audio || audio_context->audio_flush_triggered;
}
