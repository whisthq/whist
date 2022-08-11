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

#include <whist/core/whist.h>
#include <deque>
#include <algorithm>
#include "whist/utils/clock.h"
extern "C" {
#include "whist/debug/plotter.h"
#include "audio.h"
#include "network.h"
#include <whist/logging/log_statistic.h>
#include <whist/network/network.h>
#include <whist/network/ringbuffer.h>
#include <whist/core/whist_frame.h>
#include <whist/debug/protocol_analyzer.h>
#include <whist/debug/debug_console.h>
}

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

class AdaptiveParameterController;
class QueueLenController;

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

    // Audio rendering state (Buffering, or Playing)
    // TODO: make this atomic?
    AudioState audio_state;

    // Buffer for the audio buffering state
    int audio_buffering_buffer_size;
    uint8_t* audio_buffering_buffer;

    AdaptiveParameterController* adaptive_parameter_controller;
    QueueLenController* queue_len_controller;
};

/*
============================
Classes
============================
*/

// control logic class for adaptive parameter
class AdaptiveParameterController {
    // The size of the audio queue len in device that we're aiming for
    // (In frames), initial value
    const double DEVICE_QUEUE_TARGET_SIZE_INITIAL = 8;  // NOLINT
    // Total size of audio-queue and userspace buffer to overflow at
    // (In frames), initial value
    const double TOTAL_QUEUE_OVERFLOW_SIZE_INITIAL = 20;  // NOLINT

    // how large the sizes above are scaled, the initial value
    const double SCALE_FACTOR_INITIAL = 1.0;  // NOLINT
    // the min value of scale factor
    const double SCALE_FACTOR_MIN = 1.0;  // NOLINT
    // the max value of scale fator
    const double SCALE_FACTOR_MAX = 4.0;  // NOLINT
    // the step of doing a scale up, or the speed of scale up
    const double SCALE_UP_STEP = 1.5;  // NOLINT

    // when device queue len is running blow this value, it's considered risky
    const double RISKY_THRESHOLD = 2.0;  // NOLINT
    // if we have run into risk this nums, the controller triggers a scale up
    const int RISKY_CNT_BEFORE_SCALE = 3;  // NOLINT
    // if no risk observerd in this many second, the counter of risky is reset to  0
    const double RISKY_EXPIRE_TIME = 30.0;  // NOLINT

    // when device queue len is above this values it's considered safe
    const double SAFE_THRESHOLD = 4.0;  // NOLINT
    // if we have been running above SAFE_THRESHOLD for this many seconds. a scale down is triggered
    const double SAFE_DURATION = 45.0;  // NOLINT

    // a cool down time after the connection is started, in sec
    const double COOL_DOWN_FOR_STARUP = 4.0;  // NOLINT
    // a small cool down between risky status is counted, in sec
    const double COOL_DOWN_BETWEEN_RISKY_STATUS = 2.0;  // NOLINT

    // a value that used as infinite in this controller
    const double INF = 99999.0;  // NOLINT

    // the remaing cool down time, only used for scale up
    double current_cool_down;
    // last time it's considered risky, in sec
    double last_risky_time;
    // how many times risky status is observed
    int risky_cnt;

    // the min value of obeserved device queue len
    double device_queue_len_min;
    // the time from when the above min is measured
    double device_queue_len_min_since;

    // current scale factor
    std::atomic<double> current_scale_factor;

    // reset the values so that we are ready to run next round of scale logic
    int reset_for_next_round(double current_time) {
        // set initial values
        risky_cnt = 0;
        last_risky_time = current_time;
        device_queue_len_min_since = current_time;
        device_queue_len_min = INF;
        current_cool_down = COOL_DOWN_BETWEEN_RISKY_STATUS;
        return 0;
    }

    // handle scale down
    int handle_scaling_down(double device_queue_len, double current_time) {
        // maintain the value of device_queue_len_min
        if (device_queue_len < device_queue_len_min) {
            device_queue_len_min = device_queue_len;
        }

        // if we observered a value below the safe_threadhold , reset the timer and running mean,
        // so that we can re-measure from scratch
        if (device_queue_len_min < SAFE_THRESHOLD) {
            device_queue_len_min_since = current_time;
            device_queue_len_min = INF;
        }

        // if the running min has been above SAFE_THRESHOLD for SAFE_DURATION,  we shrink off the
        // spare value above SAFE_THRESHOLD
        if (device_queue_len_min > SAFE_THRESHOLD && device_queue_len_min < INF &&
            current_time - device_queue_len_min_since > SAFE_DURATION) {
            double current_audio_queue_target_size = get_device_queue_target_size();
            if (device_queue_len_min > current_audio_queue_target_size) {
                device_queue_len_min = current_audio_queue_target_size;
            }
            double spare_amount = device_queue_len_min - SAFE_THRESHOLD;
            double new_scale_factor =
                (current_audio_queue_target_size - spare_amount) / DEVICE_QUEUE_TARGET_SIZE_INITIAL;
            if (new_scale_factor < SCALE_FACTOR_MIN) {
                new_scale_factor = SCALE_FACTOR_MIN;
            }

            current_scale_factor = new_scale_factor;
            reset_for_next_round(current_time);

            return 0;
        }
        return 0;
    }

    // handle scale up
    int handle_scaling_up(double device_queue_len, double current_time) {
        // if it hasn't running low for DANGER_EXPIRE_TIME sec, reset counter
        if (current_time - last_risky_time > RISKY_EXPIRE_TIME) {
            risky_cnt = 0;
            last_risky_time = current_time;
            return 0;
        }

        // if not in cool_down, and we observed a risky status increase the count
        if (device_queue_len < RISKY_THRESHOLD &&
            current_time - last_risky_time > current_cool_down) {
            risky_cnt++;
            last_risky_time = current_time;
            current_cool_down = COOL_DOWN_BETWEEN_RISKY_STATUS;

            // if risky_cnt reached the value to trigger a scale, we perform scale up
            if (risky_cnt >= RISKY_CNT_BEFORE_SCALE) {
                double new_scale_factor = current_scale_factor * SCALE_UP_STEP;
                if (new_scale_factor > SCALE_FACTOR_MAX) {
                    new_scale_factor = SCALE_FACTOR_MAX;
                }
                current_scale_factor = new_scale_factor;
                reset_for_next_round(current_time);
            }
        }
        return 0;
    }

   public:
    // returns the max possible value of device_queue_target_size, for buffer allocation at upper
    // level
    double get_max_possible_device_queue_target_size() {
        return DEVICE_QUEUE_TARGET_SIZE_INITIAL * SCALE_FACTOR_MAX;
    }
    // init the class
    void init(double current_time) {
        current_scale_factor = SCALE_FACTOR_INITIAL;
        reset_for_next_round(current_time);
        current_cool_down = COOL_DOWN_FOR_STARUP;
    }

    // get the current scaled audio_queue_target_size
    double get_device_queue_target_size() {
        return DEVICE_QUEUE_TARGET_SIZE_INITIAL * current_scale_factor;
    }

    // get the current total_queue_overflow_size
    double get_total_queue_overflow_size() {
        return TOTAL_QUEUE_OVERFLOW_SIZE_INITIAL * current_scale_factor;
    }

    // entry function to handle both scaling down and scaling up
    int handle_scaling(double device_queue_len) {
        double current_time = get_timestamp_sec();
        // scale down and scale up are pretty much indepent
        handle_scaling_down(device_queue_len, current_time);
        handle_scaling_up(device_queue_len, current_time);
        return 0;
    }

    // get the current scale factor, exposed only for debug
    double get_scale_factor() { return current_scale_factor; }
};

class QueueLenController {
    // The time per sample,  the queue len here is the queue len of userspace + device
    const double QUEUE_LEN_SAMPLE_FREQUENCY_MS = 20;  // NOLINT

    // The max number of samples we use for average estimation
    const int QUEUE_LEN_NUM_SAMPLES_MAX = 50;  // NOLINT

    // The min number of samples we use for average estimation
    const int QUEUE_LEN_NUM_SAMPLES_MIN = 8;  // NOLINT

    // Acceptable size discrepancy that the average sample size could have
    // [DEVICE_QUEUE_TARGET_SIZE - QUEUE_LEN_ACCEPTABLE_DELTA, DEVICE_QUEUE_TARGET_SIZE +
    // QUEUE_LEN_ACCEPTABLE_DELTA]
    const double QUEUE_LEN_ACCEPTABLE_DELTA = 1.2;  // NOLINT

    // this factor affects when larger queue len control strengh is used, and how large
    const double QUEUE_LEN_CONTROL_STRENGTH_ADJUST_FACTOR =
        3 * QUEUE_LEN_ACCEPTABLE_DELTA;  // NOLINT

    // Sample sizes for average size tracking
    // Samples are measured in frames (Not necessarily whole numbers).
    // For this queue, we push from front side, and pop from back side, so that the newest sample
    // has index 0. This make code simpler, since we need to access newest samples more often
    std::deque<double> samples;
    // the timestamp in sec for last sampling
    double last_sample_time;

    // track if overflow is happening
    bool is_overflowing;

    // The adjust as recommended by size sample analysis
    // This will get set to None once it's acted upon
    std::atomic<AdjustCommand> adjust_command;

   public:
    // init the class
    void init() {
        is_overflowing = false;
        last_sample_time = 0;
        adjust_command = NOOP_FRAME;
        reset_sampling();
    }

    // reset the data structure, to sample from scratch
    void reset_sampling() { samples.clear(); }

    // make decision based on the sampling of queue len
    void handle_sampling(double current_time, double total_queue_len,
                         double device_queue_target_size) {
        if (current_time - last_sample_time <
            (double)QUEUE_LEN_SAMPLE_FREQUENCY_MS / MS_IN_SECOND) {
            return;
        }
        // Record the sample and reset the timer
        samples.push_front(total_queue_len);
        last_sample_time = current_time;

        // maintain the capicity of samples, any samples exceed QUEUE_LEN_NUM_SAMPLES_MAX is useless
        while ((int)samples.size() > QUEUE_LEN_NUM_SAMPLES_MAX) {
            samples.pop_back();
        }

        // if there is not enough samples inside, do nothing, return
        if ((int)samples.size() < QUEUE_LEN_NUM_SAMPLES_MIN) {
            return;
        }

        // maintain a running sum from newest sample to oldest smaple
        double sample_running_sum = 0;
        // add the length of [0, QUEUE_LEN_NUM_SAMPLES_MIN -1] to running sum
        for (int i = 0; i < QUEUE_LEN_NUM_SAMPLES_MIN - 1; i++) {
            sample_running_sum += samples[i];
        }

        // look at the samples in an expending window of size from QUEUE_LEN_NUM_SAMPLES_MIN to num
        // of avaliable samples, to see if there is a window of samples that triggers the queue len
        // manage strategy
        for (int current_using_sample_count = QUEUE_LEN_NUM_SAMPLES_MIN;
             current_using_sample_count <= (int)samples.size(); current_using_sample_count++) {
            // maintain the running sum by adding the current sample
            sample_running_sum += samples[current_using_sample_count - 1];
            // Calculate the running average, based on the running sum
            double sample_running_avg = sample_running_sum / current_using_sample_count;

            // calculate the distant between running_avg and target
            double distant = fabs(sample_running_avg - device_queue_target_size);

            double num_samples_needed = QUEUE_LEN_NUM_SAMPLES_MAX;
            if (distant > QUEUE_LEN_CONTROL_STRENGTH_ADJUST_FACTOR) {
                // reduce the num of required samples if the distant is too far from target
                // for example:
                // if distant is small enough, i.e. around 1.2, it needs 50 samples to support a
                // queue len control decision. if distant is 7.2 (two times the ADJUST_FACTOR), it
                // needs only 25 samples to support the decision. the larger the distant is, it
                // needs less samples to support the decision, thus reacts faster
                num_samples_needed /= distant / QUEUE_LEN_CONTROL_STRENGTH_ADJUST_FACTOR;
            }
            // make decision based on the adjusted num_samples_needed
            if (current_using_sample_count >= num_samples_needed) {
                // Check for size-target discrepancy
                if (sample_running_avg < device_queue_target_size - QUEUE_LEN_ACCEPTABLE_DELTA) {
                    if (LOG_AUDIO) {
                        LOG_INFO("[AUDIO_ALGO]Duping a frame to catch-up, %d %.2f %f\n",
                                 current_using_sample_count, sample_running_avg,
                                 device_queue_target_size);
                    }
                    adjust_command = DUP_FRAME;
                    // clear the samples, so that new operation won't be made in a period
                    reset_sampling();
                } else if (sample_running_avg >
                           device_queue_target_size + QUEUE_LEN_ACCEPTABLE_DELTA) {
                    if (LOG_AUDIO) {
                        LOG_INFO("[AUDIO_ALGO]Droping a frame to catch-up, %d %.2f %f",
                                 current_using_sample_count, sample_running_avg,
                                 device_queue_target_size);
                    }
                    adjust_command = DROP_FRAME;
                    // clear the samples, so that new operation won't be made in a period
                    reset_sampling();
                } else {
                    adjust_command = NOOP_FRAME;
                }
                // jump out the loop whenever a decision is made
                break;
            }
        }
    }

    // make decision based on the overflowing status, should be called later than handle_sampling()
    void handle_overflowing(double total_queue_len, double device_queue_target_size,
                            double total_queue_overflow_size) {
        // Check whether or not we're overflowing the audio buffer
        if (!is_overflowing && total_queue_len > total_queue_overflow_size) {
            LOG_WARNING("[AUDIO_ALGO]Audio Buffer overflowing (%.2f Frames)! Force-dropping Frames",
                        total_queue_len);
            is_overflowing = true;
        }

        // If we've returned back to normal, disable overflowing state
        if (is_overflowing && total_queue_len < device_queue_target_size + 1) {
            LOG_WARNING("[AUDIO_ALGO]Done dropping audio overflow frames (%.2f Frames)",
                        total_queue_len);
            is_overflowing = false;
        }

        // overwrite the adjust_command to DROP whenever overflow is happening.
        if (is_overflowing) {
            adjust_command = DROP_FRAME;
        }
    }

    // mark the last adjust command as consume
    void consume_last_adjust_command() { adjust_command = NOOP_FRAME; }

    AdjustCommand get_adjust_command() { return adjust_command; }
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

/**
 * @brief                          A helper function to check the audio queue len, and change state
 *                                 to buffering if necessary
 *
 * @param audio_context            The audio context to use
 *
 * @note                           it's supposed to be used only in render_audio()
 */
static void check_device_buffer_dry(AudioContext* audio_context) {
    // If the audio device runs dry, begin buffering
    // Buffering check prevents spamming this log
    if (audio_context->audio_state != BUFFERING && safe_get_audio_queue(audio_context) == 0) {
        LOG_WARNING("[AUDIO_ALGO]Audio Device is dry, will start to buffer");
        audio_context->audio_state = BUFFERING;
        whist_analyzer_record_audio_action("rebuf");
    }
}

/*
============================
Public Function Implementations
============================
*/

AudioContext* init_audio(WhistFrontend* frontend) {
    LOG_INFO("Initializing audio system targeting frontend %u", whist_frontend_get_id(frontend));

    // Allocate the audio context
    MLOCK(AudioContext* audio_context = (AudioContext*)safe_malloc(sizeof(*audio_context)),
          audio_context, sizeof(AudioContext));
    memset(audio_context, 0, sizeof(*audio_context));

    // Initialize everything
    audio_context->audio_frequency = AUDIO_FREQUENCY;
    audio_context->pending_refresh = false;
    audio_context->pending_render_context = false;
    audio_context->target_frontend = frontend;
    audio_context->audio_decoder = NULL;
    audio_context->audio_state = BUFFERING;
    audio_context->audio_buffering_buffer_size = 0;
    init_audio_player(audio_context);

    MLOCK(audio_context->adaptive_parameter_controller = new AdaptiveParameterController,
          audio_context->adaptive_parameter_controller, sizeof(AdaptiveParameterController));
    audio_context->adaptive_parameter_controller->init(get_timestamp_sec());

    MLOCK(audio_context->queue_len_controller = new QueueLenController,
          audio_context->queue_len_controller, sizeof(QueueLenController));

    audio_context->queue_len_controller->init();

    size_t abb_size =
        DECODED_BYTES_PER_FRAME *
        (audio_context->adaptive_parameter_controller->get_max_possible_device_queue_target_size() +
         1);
    MLOCK(audio_context->audio_buffering_buffer = (uint8_t*)safe_malloc(abb_size),
          audio_context->audio_buffering_buffer, abb_size);
    // Return the audio context
    return audio_context;
}

void destroy_audio(AudioContext* audio_context) {
    LOG_INFO("Destroying audio system targeting frontend %u",
             whist_frontend_get_id(audio_context->target_frontend));

    MUNLOCK(delete audio_context->adaptive_parameter_controller,
            audio_context->adaptive_parameter_controller, sizeof(AdaptiveParameterController));
    MUNLOCK(delete audio_context->queue_len_controller, audio_context->queue_len_controller,
            sizeof(QueueLenController));

    // Destroy the audio device
    destroy_audio_player(audio_context);
    // Destory the buffer for buffering state
    MUNLOCK(free(audio_context->audio_buffering_buffer), audio_context->audio_buffering_buffer,
            DECODED_BYTES_PER_FRAME * (audio_context->adaptive_parameter_controller
                                           ->get_max_possible_device_queue_target_size() +
                                       1));
    // Free the audio struct
    MUNLOCK(free(audio_context), audio_context, sizeof(AudioContext));
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
        LOG_INFO("pending_render_context= %d", (int)audio_context->pending_render_context);
    }

    // make a reference with shorter name for convinence
    auto& adaptive_parameter_controller = *audio_context->adaptive_parameter_controller;
    auto& queue_len_controller = *audio_context->queue_len_controller;

    // Get device size and total buffer size
    int audio_device_len_in_bytes = (int)safe_get_audio_queue(audio_context);
    int audio_total_len_in_bytes =
        audio_device_len_in_bytes + num_frames_buffered * DECODED_BYTES_PER_FRAME;

    // the value of queue lens in num of frames
    double device_queue_len = audio_device_len_in_bytes / (double)DECODED_BYTES_PER_FRAME;
    double total_queue_len = audio_total_len_in_bytes / (double)DECODED_BYTES_PER_FRAME;

    whist_analyzer_record_current_audio_queue_info(adaptive_parameter_controller.get_scale_factor(),
                                                   total_queue_len - device_queue_len,
                                                   device_queue_len);
    if (PLOT_AUDIO_ALGO || get_debug_console_override_values()->plot_audio_algo) {
        double current_time = get_timestamp_sec();
        whist_plotter_insert_sample("audio_device_queue", current_time, device_queue_len);
        whist_plotter_insert_sample("audio_total_queue", current_time, total_queue_len);
        whist_plotter_insert_sample("audio_scale_factor", current_time,
                                    adaptive_parameter_controller.get_scale_factor());
    }

    if (LOG_AUDIO) {
        LOG_INFO_RATE_LIMITED(0.1, 1,
                              "[AUDIO_ALGO]queue_len: %d %.2f  state=%d  scale_factor=%.2f\n",
                              num_frames_buffered, device_queue_len, audio_context->audio_state,
                              adaptive_parameter_controller.get_scale_factor());
    } else {
        LOG_INFO_RATE_LIMITED(5, 1, "[AUDIO_ALGO]queue_len: %d %.2f  state=%d  scale_factor=%.2f\n",
                              num_frames_buffered, device_queue_len, audio_context->audio_state,
                              adaptive_parameter_controller.get_scale_factor());
    }

    adaptive_parameter_controller.handle_scaling(device_queue_len);

    // get the current values of queue lens from adaptive_parameter_controller
    double current_device_queue_target_size =
        adaptive_parameter_controller.get_device_queue_target_size();
    double current_total_queue_overflow_size =
        adaptive_parameter_controller.get_total_queue_overflow_size();

    // Every sample freq ms of playtime, record an audio sample
    if (audio_context->audio_state == PLAYING) {
        queue_len_controller.handle_sampling(get_timestamp_sec(), total_queue_len,
                                             current_device_queue_target_size);
    } else {
        // Clear sample-tracking if we're buffering
        queue_len_controller.reset_sampling();
    }

    // Always drop if we're overflowing
    queue_len_controller.handle_overflowing(total_queue_len, current_device_queue_target_size,
                                            current_total_queue_overflow_size);

    // Consume a new frame, if the renderer has room to queue frames,
    // or if we need to drop a frame
    int num_frames_to_render = queue_len_controller.get_adjust_command() == DUP_FRAME ? 2 : 1;
    bool wants_new_frame =
        !audio_context->pending_render_context &&
        audio_device_len_in_bytes <=
            (current_device_queue_target_size - num_frames_to_render) * DECODED_BYTES_PER_FRAME;
    return wants_new_frame || queue_len_controller.get_adjust_command() == DROP_FRAME;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
void receive_audio(AudioContext* audio_context, AudioFrame* audio_frame) {
    // make a reference with shorter name for convinence
    auto& queue_len_controller = *audio_context->queue_len_controller;

    if (LOG_AUDIO) {
        if (queue_len_controller.get_adjust_command() == DUP_FRAME) {
            LOG_INFO("Receiving Audio [Duping Frame]");
        } else if (queue_len_controller.get_adjust_command() == DROP_FRAME) {
            LOG_INFO("Receiving Audio [Dropping Frame]");
        } else {
            LOG_INFO("Receiving Audio");
        }
    }

    if (PLOT_AUDIO_ALGO || get_debug_console_override_values()->plot_audio_algo) {
        double current_time = get_timestamp_sec();
        whist_plotter_insert_sample(
            "audio_dup", current_time,
            queue_len_controller.get_adjust_command() == DUP_FRAME ? 1.0 : 0.0);
        whist_plotter_insert_sample(
            "audio_drop", current_time,
            queue_len_controller.get_adjust_command() == DROP_FRAME ? 1.0 : 0.0);
    }

    // If we're supposed to drop it, drop it.
    if (queue_len_controller.get_adjust_command() == DROP_FRAME) {
        log_double_statistic(AUDIO_FRAMES_SKIPPED, 1.0);
        // logically we consider dropped frames as pending first
        whist_analyzer_record_pending_rendering(PACKET_AUDIO);
        whist_analyzer_record_audio_action("drop");
    } else {
        // If we shouldn't drop it, push the audio frame to the render context
        if (!audio_context->pending_render_context) {
            // Mark out the audio frame to the render context
            audio_context->render_context.audio_frame = audio_frame;
            audio_context->render_context.adjust_command =
                queue_len_controller.get_adjust_command();

            // LOG_DEBUG("received packet with ID %d, audio last played %d", packet->id,
            // audio_context->last_played_id); signal to the renderer that we're ready

            whist_analyzer_record_pending_rendering(PACKET_AUDIO);
            if (audio_context->render_context.adjust_command == DUP_FRAME) {
                whist_analyzer_record_audio_action("dup");
            }
            audio_context->pending_render_context = true;
        } else {
            LOG_ERROR("We tried to render audio, but the renderer wasn't ready!");
        }
    }
    // Mark the command as consumed
    queue_len_controller.consume_last_adjust_command();
}

void render_audio(AudioContext* audio_context) {
    auto& adaptive_parameter_controller = *audio_context->adaptive_parameter_controller;

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

            // check if buffer is dry and change state to BUFFERING if necessary
            check_device_buffer_dry(audio_context);

            // If we should dup the frame, resend it into the decoder
            if (audio_context->render_context.adjust_command == DUP_FRAME &&
                audio_context->audio_state != BUFFERING) {
                if (LOG_AUDIO) {
                    LOG_INFO("[AUDIO_ALGO]Dup a frame at the decoder\n");
                }
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

                // check if buffer is dry again, since the status might have changed since last
                // check
                check_device_buffer_dry(audio_context);

                // If we're buffering, then buffer
                if (audio_context->audio_state == BUFFERING) {
                    if (LOG_AUDIO) {
                        LOG_INFO("Flushing Audio Buffer to device");
                    }
                    // If it's large enough to hit the target, start playing it all
                    if (audio_context->audio_buffering_buffer_size + (int)decoded_data_size >
                        (adaptive_parameter_controller.get_device_queue_target_size() - 1) *
                            DECODED_BYTES_PER_FRAME) {
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
