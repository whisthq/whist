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
#define AUDIO_QUEUE_TARGET_SIZE_0 8
// The total size of audio-queue and userspace buffer to overflow at
// In frames
#define AUDIO_BUFFER_OVERFLOW_SIZE_0 20

int AUDIO_QUEUE_TARGET_SIZE = AUDIO_QUEUE_TARGET_SIZE_0;
int AUDIO_BUFFER_OVERFLOW_SIZE = AUDIO_BUFFER_OVERFLOW_SIZE_0;

// The time per size sample
#define AUDIO_BUFSIZE_SAMPLE_FREQUENCY_MS 20

// The number of samples we use for average estimation
#define AUDIO_BUFSIZE_NUM_SAMPLES 50

// Acceptable size discrepancy that the average sample size could have
// [AUDIO_QUEUE_TARGET_SIZE-AUDIO_ACCEPTABLE_DELTA, AUDIO_QUEUE_TARGET_SIZE+AUDIO_ACCEPTABLE_DELTA]
#define AUDIO_ACCEPTABLE_DELTA 1.2

#define scale_each_time 1.5
#define init_scale_factor 1.0
#define scale_max 4.0
double scale_factor = init_scale_factor;

double danger_threshold = 2.0;
double safe_threshold = 4.0;

WhistTimer my_timer;
double cool_down;
double last_running_low_time;
int running_low_cnt = 0;
int cool_down_large = 5.0;
int cool_down_small = 4.0;

double scale_down_last_check_time;
double running_min;
double inf = 99999.0;

static int adjust_to_scale_factor(double current_time) {
    AUDIO_QUEUE_TARGET_SIZE = AUDIO_QUEUE_TARGET_SIZE_0 * scale_factor;
    AUDIO_BUFFER_OVERFLOW_SIZE = AUDIO_BUFFER_OVERFLOW_SIZE_0 * scale_factor;

    running_low_cnt = 0;
    last_running_low_time = current_time;
    scale_down_last_check_time = current_time;
    running_min = inf;
    cool_down = cool_down_small;

    return 0;
}

static int dynamic_scaling_reinit() {
    // scale_factor= init_scale_factor;
    start_timer(&my_timer);
    double current_time = get_timer(&my_timer);
    adjust_to_scale_factor(current_time);

    cool_down = cool_down_large;
    return 0;
}

static int handle_scale_down(double device_queue_len, double current_time) {
    // maintain the running min
    if (device_queue_len < running_min) {
        running_min = device_queue_len;
    }

    // if we observered a value below the safe_threadhold , reset the timer and running mean,
    // so that we can re-measure from scratch
    if (running_min < safe_threshold) {
        scale_down_last_check_time = current_time;
        running_min = inf;
    }

    // if the running min has been above safe thres_hold,  we decrease queue len according to the
    // threshold
    if (running_min > safe_threshold && running_min < inf - 1 &&
        current_time - scale_down_last_check_time > 45.0) {
        double adjust = running_min - safe_threshold;
        double new_scale_factor = (AUDIO_QUEUE_TARGET_SIZE - adjust) / AUDIO_QUEUE_TARGET_SIZE_0;
        if (new_scale_factor < 0.8) new_scale_factor = 0.8;

        // TODO: make sure scale down doesn't cause overflow
        scale_factor = new_scale_factor;
        adjust_to_scale_factor(current_time);

        return 0;
    }
    return 0;
}

static int handle_scale_up(double device_queue_len, double current_time) {
    // if it hasn't running low for 30s, reset counter
    if (current_time - last_running_low_time > 30) {
        running_low_cnt = 0;
        last_running_low_time = current_time;
        return 0;
    }

    if (device_queue_len < danger_threshold && current_time - last_running_low_time > cool_down) {
        running_low_cnt++;
        last_running_low_time = current_time;
        cool_down = cool_down_small;

        if (running_low_cnt >= 3) {
            scale_factor *= scale_each_time;
            if (scale_factor > scale_max) scale_factor = scale_max;
            adjust_to_scale_factor(current_time);
        }
    }
    return 0;
}

static int handle_dynamic_scaling(double device_queue_len) {
    double current_time = get_timer(&my_timer);
    // scale down and scale up are pretty much indepent
    handle_scale_down(device_queue_len, current_time);
    handle_scale_up(device_queue_len, current_time);

    return 0;
}
/*
============================
Custom Types
============================
*/

// Whether or not we're buffering, or playing
typedef enum {
    BUFFERING,
    PLAYING,
} AudioState;

// Whether we should dup a frame, or drop a frame
typedef enum {
    NOOP_FRAME,
    DROP_FRAME,
    DUP_FRAME,
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

    // The audio decoder
    AudioDecoder* audio_decoder;
    // The frontend to play audio with
    WhistFrontend* target_frontend;

    // The audio render context data
    AudioRenderContext render_context;
    // Whether or not the render context is populated and ready-to-be-played
    bool pending_render_context;

    // Sample sizes for average size tracking
    // Samples are measured in frames (Not necessarily whole numbers)
    WhistTimer size_sample_timer;
    int sample_index;
    double samples[AUDIO_BUFSIZE_NUM_SAMPLES];
    // The adjust as recommended by size sample analysis
    // This will get set to None once it's acted upon
    AdjustCommand adjust_command;

    // Audio rendering state (Buffering, or Playing)
    AudioState audio_state;
    // Overflow state
    bool is_overflowing;
    // Buffer for the audio buffering state
    int audio_buffering_buffer_size;
    uint8_t audio_buffering_buffer[DECODED_BYTES_PER_FRAME * (999 + 1)];
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
    audio_context->audio_state = BUFFERING;
    audio_context->is_overflowing = false;
    audio_context->audio_buffering_buffer_size = 0;
    init_audio_player(audio_context);

    // Init sample size info
    audio_context->adjust_command = NOOP_FRAME;
    start_timer(&audio_context->size_sample_timer);
    audio_context->sample_index = 0;

    dynamic_scaling_reinit();

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

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
bool audio_ready_for_frame(AudioContext* audio_context, int num_frames_buffered) {
    if (get_debug_console_override_values()->verbose_log_audio) {
        LOG_INFO("[audio]pending_render_context= %d", (int)audio_context->pending_render_context);
    }

    // Get device size and total buffer size
    int audio_device_size = (int)safe_get_audio_queue(audio_context);
    int audio_size = audio_device_size + num_frames_buffered * DECODED_BYTES_PER_FRAME;

    // Every sample freq ms of playtime, record an audio sample
    if (audio_context->audio_state == PLAYING) {
        double device_queue_len = audio_device_size / (double)DECODED_BYTES_PER_FRAME;

        handle_dynamic_scaling(device_queue_len);

        if (get_timer(&audio_context->size_sample_timer) * MS_IN_SECOND >
            AUDIO_BUFSIZE_SAMPLE_FREQUENCY_MS) {
            static int log_cnt = 0;
            log_cnt++;
            if (log_cnt % 5 == 0)
                fprintf(stderr, "current: %.2d %.2f %.2f\n", num_frames_buffered, device_queue_len,
                        scale_factor);

            // Record the sample and reset the timer
            audio_context->samples[audio_context->sample_index] =
                audio_size / (double)DECODED_BYTES_PER_FRAME;
            audio_context->sample_index++;
            start_timer(&audio_context->size_sample_timer);
            if (LOG_AUDIO) {
                LOG_INFO("Audio Buffer Size: %.2f [Device %.2f + Buffer %d]",
                         audio_size / (double)DECODED_BYTES_PER_FRAME,
                         audio_device_size / (double)DECODED_BYTES_PER_FRAME, num_frames_buffered);
            }

            if (audio_context->sample_index >= 7) {
                int sample_count = audio_context->sample_index;
                // Calculate the new average, in fractional frames
                double avg_sample = 0.0;
                for (int i = 0; i < sample_count; i++) {
                    avg_sample += audio_context->samples[i];
                }

                avg_sample /= sample_count;

                double distant = fabs(avg_sample - AUDIO_QUEUE_TARGET_SIZE);
                double num_samples_needed = AUDIO_BUFSIZE_NUM_SAMPLES;
                if (distant > 3 * AUDIO_ACCEPTABLE_DELTA) {
                    // make the required delta smaller if distant is too
                    num_samples_needed /= (distant / 3 * AUDIO_ACCEPTABLE_DELTA);
                }

                if (sample_count >= num_samples_needed) {
                    audio_context->sample_index = 0;
                    // Check for size-target discrepancy
                    if (avg_sample < AUDIO_QUEUE_TARGET_SIZE - AUDIO_ACCEPTABLE_DELTA) {
                        fprintf(stderr, "dup a frame, %d %.2f %d\n", sample_count, avg_sample,
                                AUDIO_QUEUE_TARGET_SIZE);
                        if (LOG_AUDIO) {
                            LOG_INFO("Duping a frame to catch-up");
                        }
                        audio_context->adjust_command = DUP_FRAME;
                    }
                    if (avg_sample > AUDIO_QUEUE_TARGET_SIZE + AUDIO_ACCEPTABLE_DELTA) {
                        fprintf(stderr, "drop a frame, %d %.2f %d\n", sample_count, avg_sample,
                                AUDIO_QUEUE_TARGET_SIZE);
                        if (LOG_AUDIO) {
                            LOG_INFO("Droping a frame to catch-up");
                        }
                        audio_context->adjust_command = DROP_FRAME;
                    }
                }
                // always clear if num sample is full
                else if (audio_context->sample_index == AUDIO_BUFSIZE_NUM_SAMPLES) {
                    audio_context->sample_index = 0;
                    if (LOG_AUDIO) {
                        LOG_INFO("Audio Buffer Average Size: %.2f", avg_sample);
                    }
                }
            }
        }
    } else {
        // Clear sample-tracking if we're buffering
        audio_context->sample_index = 0;
    }

    // Check whether or not we're overflowing the audio buffer
    if (!audio_context->is_overflowing &&
        audio_size > AUDIO_BUFFER_OVERFLOW_SIZE * DECODED_BYTES_PER_FRAME) {
        LOG_WARNING("Audio Buffer overflowing (%.2f Frames)! Force-dropping Frames",
                    audio_size / (double)DECODED_BYTES_PER_FRAME);
        fprintf(stderr, "audio buffer overflowing!!!!!\n");
        audio_context->is_overflowing = true;
    }

    // If we've returned back to normal, disable overflowing state
    if (audio_context->is_overflowing &&
        audio_size < (AUDIO_QUEUE_TARGET_SIZE + 1) * DECODED_BYTES_PER_FRAME) {
        LOG_WARNING("Done dropping audio overflow frames (Buffer size: %.2f)",
                    audio_size / (double)DECODED_BYTES_PER_FRAME);
        audio_context->is_overflowing = false;
    }

    // Always drop if we're overflowing
    if (audio_context->is_overflowing) {
        audio_context->adjust_command = DROP_FRAME;
    }

    // Consume a new frame, if the renderer has room to queue 8 frames,
    // or if we need to drop a frame
    int num_frames_to_render = audio_context->adjust_command == DUP_FRAME ? 2 : 1;
    bool wants_new_frame = !audio_context->pending_render_context &&
                           audio_device_size <= (AUDIO_QUEUE_TARGET_SIZE - num_frames_to_render) *
                                                    DECODED_BYTES_PER_FRAME;
    return wants_new_frame || audio_context->adjust_command == DROP_FRAME;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
void receive_audio(AudioContext* audio_context, AudioFrame* audio_frame) {
    if (LOG_AUDIO) {
        if (audio_context->adjust_command == DUP_FRAME) {
            LOG_INFO("Receiving Audio [Duping Frame]");
        } else if (audio_context->adjust_command == DROP_FRAME) {
            LOG_INFO("Receiving Audio [Dropping Frame]");
        } else {
            LOG_INFO("Receiving Audio");
        }
    }
    // If we're supposed to drop it, drop it.
    if (audio_context->adjust_command == DROP_FRAME) {
        log_double_statistic(AUDIO_FRAMES_SKIPPED, 1.0);
    } else {
        // If we shouldn't drop it, push the audio frame to the render context
        if (!audio_context->pending_render_context) {
            // Mark out the audio frame to the render context
            audio_context->render_context.audio_frame = audio_frame;
            audio_context->render_context.adjust_command = audio_context->adjust_command;

            // LOG_DEBUG("received packet with ID %d, audio last played %d", packet->id,
            // audio_context->last_played_id); signal to the renderer that we're ready

            whist_analyzer_record_pending_rendering(PACKET_AUDIO);
            audio_context->pending_render_context = true;
        } else {
            LOG_ERROR("We tried to render audio, but the renderer wasn't ready!");
        }
    }
    // Mark the command as consumed
    audio_context->adjust_command = NOOP_FRAME;
}

void render_audio(AudioContext* audio_context) {
    if (audio_context->pending_render_context) {
        if (LOG_AUDIO) {
            LOG_INFO("Rendering Audio");
        }

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
            LOG_INFO("Refreshing Audio Device");
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
            // If we should dup the frame, resend it into the decoder
            if (audio_context->render_context.adjust_command == DUP_FRAME) {
                // Duping via the decoder sounds better,
                // It must be smoothing it somehow with some unknown method
                if (audio_decoder_send_packets(audio_context->audio_decoder, audio_frame->data,
                                               audio_frame->data_length) < 0) {
                    LOG_ERROR("Failed to send duplicated packets to the decoder!");
                }
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

                // If the audio device runs dry, begin buffering
                // Buffering check prevents spamming this log
                if (audio_context->audio_state != BUFFERING &&
                    safe_get_audio_queue(audio_context) == 0) {
                    LOG_WARNING("Audio Device is dry, will start to buffer");
                    fprintf(stderr, "device buffer dry !!!\n");
                    audio_context->audio_state = BUFFERING;
                }

                // If we're buffering, then buffer
                if (audio_context->audio_state == BUFFERING) {
                    if (LOG_AUDIO) {
                        LOG_INFO("Flushing Audio Buffer to device");
                    }
                    // If it's large enough to hit the target, start playing it all
                    if (audio_context->audio_buffering_buffer_size + (int)decoded_data_size >
                        (AUDIO_QUEUE_TARGET_SIZE - 1) * DECODED_BYTES_PER_FRAME) {
                        whist_frontend_queue_audio(
                            audio_context->target_frontend, audio_context->audio_buffering_buffer,
                            (size_t)audio_context->audio_buffering_buffer_size);
                        audio_context->audio_buffering_buffer_size = 0;
                        whist_frontend_queue_audio(audio_context->target_frontend, decoded_data,
                                                   decoded_data_size);
                        audio_context->audio_state = PLAYING;
                    } else {
                        if (LOG_AUDIO) {
                            LOG_INFO("Buffering Audio Frame...");
                        }
                        // Otherwise, keep buffering
                        memcpy(audio_context->audio_buffering_buffer +
                                   audio_context->audio_buffering_buffer_size,
                               decoded_data, decoded_data_size);
                        audio_context->audio_buffering_buffer_size += (int)decoded_data_size;
                    }
                } else {
                    // If we're playing, then play the audio
                    whist_frontend_queue_audio(audio_context->target_frontend, decoded_data,
                                               decoded_data_size);
                }
            }
        }

        if (LOG_AUDIO) {
            LOG_INFO("Done Rendering Audio (%.2f Device Size)",
                     safe_get_audio_queue(audio_context) / (double)DECODED_BYTES_PER_FRAME);
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
    return audio_queue;
}
