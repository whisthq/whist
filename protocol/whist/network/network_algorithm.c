/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network_algorithm.c
 * @brief This file contains the client's adaptive bitrate code. Any algorithms we are using for
 *        predicting bitrate should be stored here.
============================
Usage
============================
Place to put any predictive/adaptive bitrate algorithms. In the current setup, each algorithm is a
function that takes in inputs through a NetworkStatistics struct. The function is responsible for
maintaining any internal state necessary for the algorithm (possibly through the use of custom types
or helpers), and should update client_max_bitrate when necessary.

To change the algorithm the client uses, set `calculate_new_bitrate` to the desired algorithm.
The function assigned to `calculate_new_bitrate` should have signature such that it takes
one argument of type `NetworkStatistics` and returns type `NetworkSettings`.

For more details about the individual algorithms, please visit
https://www.notion.so/whisthq/Adaptive-Bitrate-Algorithms-a6ea0987adc04e8d84792fc9dcbc7fc7
*/

/*
============================
Includes
============================
*/

#include "network_algorithm.h"
#include "whist/debug/debug_console.h"
#include "whist/core/features.h"

/*
============================
Globals
============================
*/

volatile int output_width;
volatile int output_height;
volatile bool insufficient_bandwidth;

/*
============================
Defines
============================
*/

// bitrate and burst_bitrate doesn't have any compile-time default values as it depends on the
// resolution
static NetworkSettings default_network_settings = {
    .desired_codec = CODEC_TYPE_H264,
    .audio_fec_ratio = AUDIO_FEC_RATIO,
    .video_fec_ratio = VIDEO_FEC_RATIO,
    .fps = 60,
};

#define DPI_BITRATE_PER_PIXEL 192

#define EWMA_STATS_SECONDS 5

// This value directly impacts the time to transmit big frames. Also this value indirectly impacts
// the video quality as the VBV buffer size is based on this ratio. Also if this value is set too
// high then we will observe packet loss. So it is tradeoff between packet loss vs latency/quality
#define BURST_BITRATE_RATIO 4

// This value was decided heuristically by visually comparing similar resolution windows in 192 DPI
// and 96 DPI screens. This translates to 96 DPI screens having a bitrate ~3x more than 192 DPI
// screen for same resolution.
#define DPI_RATIO_EXPONENT 1.6

#define MINIMUM_BITRATE \
    get_video_bitrate(output_width, output_height, dpi, MINIMUM_BITRATE_PER_PIXEL)
#define MAXIMUM_BITRATE \
    get_video_bitrate(output_width, output_height, dpi, MAXIMUM_BITRATE_PER_PIXEL)
#define STARTING_BITRATE \
    get_video_bitrate(output_width, output_height, dpi, STARTING_BITRATE_PER_PIXEL)

#define MINIMUM_BURST_BITRATE (MINIMUM_BITRATE * BURST_BITRATE_RATIO)
#define MAXIMUM_BURST_BITRATE (MAXIMUM_BITRATE * BURST_BITRATE_RATIO)
#define STARTING_BURST_BITRATE (STARTING_BITRATE * BURST_BITRATE_RATIO)

static int dpi = -1;

/*
============================
Private Function Declarations
============================
*/

static int get_video_bitrate(int width, int height, int screen_dpi, double bitrate_per_pixel) {
    // We have to scale-up the bitrates for lower DPI screens. This is because Chrome renders more
    // content in lower DPI screens and hence higher level of details. So the same bitrate cannot be
    // used for screens of different DPIs
    // Also limit the DPI ratio, so that we don't get crazy values
    double dpi_ratio = max(min((double)DPI_BITRATE_PER_PIXEL / screen_dpi, 2.0), 0.5);
    double dpi_scaling_factor = pow(dpi_ratio, DPI_RATIO_EXPONENT);
    return (int)(width * height * bitrate_per_pixel * dpi_scaling_factor);
}

/*
============================
Public Function Implementations
============================
*/

NetworkSettings get_default_network_settings(int width, int height, int screen_dpi) {
    FATAL_ASSERT(width > 0 && height > 0 && screen_dpi > 0);
    default_network_settings.video_bitrate =
        get_video_bitrate(width, height, screen_dpi, STARTING_BITRATE_PER_PIXEL);
    // Saturate bitrate initially, so that we can find the max bandwidth quickly
    default_network_settings.saturate_bandwidth = true;
    // Whist congestion control increases burst bitrate only after exceeding max bitrate
    default_network_settings.burst_bitrate = default_network_settings.video_bitrate;
    return default_network_settings;
}

NetworkSettings get_starting_network_settings(void) {
    return get_default_network_settings(output_width, output_height, dpi);
}

void network_algo_set_dpi(int new_dpi) { dpi = new_dpi; }

// The theory behind all the code in this function is documented in WCC.md file. Please go thru that
// document before reviewing this file. Also if you make any modifications to this algo, remember to
// update the WCC.md file as well so that the documentation remains upto date.
bool whist_congestion_controller(GroupStats *curr_group_stats, GroupStats *prev_group_stats,
                                 int incoming_bitrate, double packet_loss_ratio,
                                 double short_term_latency, double long_term_latency,
                                 NetworkSettings *network_settings) {
// Latest delay variation gets this weightage. Older value gets a weightage of (1 - EWMA_FACTOR)
#define EWMA_FACTOR 0.3
// Latest max bitrate gets this weightage.
#define MAX_BITRATE_EWMA_FACTOR 0.05
#define DELAY_VARIATION_THRESHOLD_IN_SEC 0.01   // 10ms
#define LONG_OVERUSE_TIME_THRESHOLD_IN_SEC 0.1  // 100ms
#define DECREASE_RATIO 0.95
#define BANDWITH_USED_THRESHOLD 0.95
// Higher value will mean increased latency. Lower value will increase false positives for
// congestion. Right now set to 50ms based on tradeoff between acceptable E2E latency vs false
// positives.
#define MIN_LATENCY_THRESHOLD_SEC 0.05  // 50ms
    if (incoming_bitrate <= 0) {
        // Not enough data to take any decision. Let the bits start flowing.
        return false;
    }

    static WhistTimer overuse_timer;
    static WhistTimer last_update_timer;
    static WhistTimer last_decrease_timer;
    static bool delay_controller_initialized = false;

    static double filtered_delay_variation;
    static double increase_percentage = MAX_INCREASE_PERCENTAGE;
    static bool burst_mode = false;
    static int max_bitrate_available = 0;
    static int last_successful_bitrate = 0;

    if (!delay_controller_initialized) {
        start_timer(&overuse_timer);
        start_timer(&last_update_timer);
        start_timer(&last_decrease_timer);
    }
    int max_bitrate = MAXIMUM_BITRATE;
    int new_bitrate = network_settings->video_bitrate;

    enum {
        UNDERUSE_SIGNAL,
        NORMAL_SIGNAL,
        OVERUSE_SIGNAL,
    } overuse_detector_signal = NORMAL_SIGNAL;

    static enum {
        DELAY_CONTROLLER_INCREASE,
        DELAY_CONTROLLER_HOLD,
        DELAY_CONTROLLER_DECREASE
    } delay_controller_state = DELAY_CONTROLLER_HOLD;

    double inter_departure_time =
        (double)(curr_group_stats->departure_time - prev_group_stats->departure_time) /
        US_IN_SECOND;
    double inter_arrival_time =
        (double)(curr_group_stats->arrival_time - prev_group_stats->arrival_time) / US_IN_SECOND;
    double delay_variation = inter_arrival_time - inter_departure_time;

    if (!delay_controller_initialized) {
        filtered_delay_variation = delay_variation;
    } else {
        filtered_delay_variation =
            filtered_delay_variation * (1.0 - EWMA_FACTOR) + delay_variation * EWMA_FACTOR;
    }

    delay_controller_initialized = true;

    static bool maybe_overuse = false;
    // Detect over/under use using state machine outlined in spec
    // In burst mode delay gradient might increase, but it doesn't mean congestion. In burst
    // mode we will rely for packet loss ratio for overuse detection
    if ((DELAY_VARIATION_THRESHOLD_IN_SEC < filtered_delay_variation && !burst_mode) ||
        packet_loss_ratio > 0.1) {
        if (!maybe_overuse) {
            start_timer(&overuse_timer);
            maybe_overuse = true;
            LOG_INFO("Maybe overuse!, filtered_delay_variation = %0.3f, packet_loss_ratio = %.2f",
                     filtered_delay_variation, packet_loss_ratio);
        }
        double overuse_time_threshold;
        if (network_settings->saturate_bandwidth) {
            overuse_time_threshold = OVERUSE_TIME_THRESHOLD_IN_SEC;
        } else {
            // When saturate bandwidth is off, bitflow will be low during idle frames and spiky in
            // non-idle frames. To ensure averages are not skewed due to 1 or 2 spiky frames,
            // we wait for a longer duration to confirm overuse.
            overuse_time_threshold = LONG_OVERUSE_TIME_THRESHOLD_IN_SEC;
        }
        // A definitive over-use will be signaled only if over-use has been
        // detected for at least overuse_time_th milliseconds.  However, if m(i)
        // < m(i-1), over-use will not be signaled even if all the above
        // conditions are met.
        if (get_timer(&overuse_timer) > overuse_time_threshold) {
            overuse_detector_signal = OVERUSE_SIGNAL;
        } else {
            // If neither over-use nor under-use is detected, the detector will be in the normal
            // state.
            overuse_detector_signal = NORMAL_SIGNAL;
        }
    } else if (-DELAY_VARIATION_THRESHOLD_IN_SEC > filtered_delay_variation) {
        overuse_detector_signal = UNDERUSE_SIGNAL;
        maybe_overuse = false;
    } else {
        overuse_detector_signal = NORMAL_SIGNAL;
        maybe_overuse = false;
    }

    // The state transitions (with blank fields meaning "remain in state")
    // are:
    // +----+--------+-----------+------------+--------+
    // |     \ State |   Hold    |  Increase  |Decrease|
    // |      \      |           |            |        |
    // | Signal\     |           |            |        |
    // +--------+----+-----------+------------+--------+
    // |  Over-use   | Decrease  |  Decrease  |        |
    // +-------------+-----------+------------+--------+
    // |  Normal     | Increase  |            |  Hold  |
    // +-------------+-----------+------------+--------+
    // |  Under-use  |           |   Hold     |  Hold  |
    // +-------------+-----------+------------+--------+
    if (overuse_detector_signal == OVERUSE_SIGNAL) {
        delay_controller_state = DELAY_CONTROLLER_DECREASE;
    } else if (overuse_detector_signal == NORMAL_SIGNAL) {
        if (delay_controller_state == DELAY_CONTROLLER_HOLD) {
            delay_controller_state = DELAY_CONTROLLER_INCREASE;
        } else if (delay_controller_state == DELAY_CONTROLLER_DECREASE) {
            delay_controller_state = DELAY_CONTROLLER_HOLD;
        }
    } else if (overuse_detector_signal == UNDERUSE_SIGNAL) {
        delay_controller_state = DELAY_CONTROLLER_HOLD;
    }

    // If the latency suddenly increases, then it might mean congestion due to longer queue length.
    // A latency based threshold check is added as delay_variation many times fails to detect this
    // queue length increase.
    double latency_threshold_decrease_state;
    double latency_threshold_hold_state;
#define LATENCY_MULTIPLIER_BURST_MODE 2.0
#define LATENCY_MULTIPLIER_NORMAL_MODE 1.3
#define LATENCY_MULTIPLIER_HOLD_STATE 1.1
    // Using a higher latency threshold for burst mode, as sending packets in a burst can
    // momentarily cause congestion that will get cleared up immediately.
    if (burst_mode)
        latency_threshold_decrease_state = long_term_latency * LATENCY_MULTIPLIER_BURST_MODE;
    else
        latency_threshold_decrease_state = long_term_latency * LATENCY_MULTIPLIER_NORMAL_MODE;
    latency_threshold_hold_state = long_term_latency * LATENCY_MULTIPLIER_HOLD_STATE;
    // For very low latency networks, jitter might be wrongly detected as congestion. Hence using
    // this MIN_LATENCY_THRESHOLD_SEC absolute threshold.
    latency_threshold_decrease_state =
        max(latency_threshold_decrease_state, MIN_LATENCY_THRESHOLD_SEC);
    latency_threshold_hold_state = max(latency_threshold_hold_state, MIN_LATENCY_THRESHOLD_SEC);

    if (short_term_latency > latency_threshold_decrease_state) {
        delay_controller_state = DELAY_CONTROLLER_DECREASE;
    } else if (short_term_latency > latency_threshold_hold_state) {
        delay_controller_state = DELAY_CONTROLLER_HOLD;
    }

    // Delay-based controller selects based on overuse signal
    // It is RECOMMENDED to send the REMB message as soon
    // as congestion is detected, and otherwise at least once every second.
    // It is RECOMMENDED that the routine to update A_hat(i) is run at least
    // once every response_time interval.
    if (delay_controller_state == DELAY_CONTROLLER_INCREASE &&
        get_timer(&last_update_timer) > NEW_BITRATE_DURATION_IN_SEC &&
        incoming_bitrate > (network_settings->video_bitrate * BANDWITH_USED_THRESHOLD)) {
        last_successful_bitrate = network_settings->video_bitrate;
        // Looks like the network has found a new max bitrate. Let find the new max bandwidth.
        if (last_successful_bitrate > max_bitrate_available) {
            max_bitrate_available = last_successful_bitrate;
            network_settings->saturate_bandwidth = true;
            increase_percentage = MAX_INCREASE_PERCENTAGE;
        }
        // If in saturate bandwidth mode and no congestion is detected, then increase percentage
        // should be made higher to quickly find the new max bitrate
        if (network_settings->saturate_bandwidth == true) {
            increase_percentage = MAX_INCREASE_PERCENTAGE;
        }
        LOG_INFO("Increase bitrate by %.3f percent", increase_percentage);
        new_bitrate = network_settings->video_bitrate * (1.0 + increase_percentage / 100.0);
    } else if ((delay_controller_state == DELAY_CONTROLLER_DECREASE) &&
               get_timer(&last_decrease_timer) > NEW_BITRATE_DURATION_IN_SEC) {
        LOG_INFO(
            "Decrease bitrate filtered_delay_variation = %.3f packet_loss_ratio = %.2f, "
            "short_term_latency = %0.3f, long_term_latency = %.3f",
            filtered_delay_variation, packet_loss_ratio, short_term_latency * MS_IN_SECOND,
            long_term_latency * MS_IN_SECOND);
        // Decrease the max_bitrate_available gradually, if congestion is detected at a lower
        // bitrate
        if (last_successful_bitrate < max_bitrate_available) {
            max_bitrate_available = max_bitrate_available * (1.0 - MAX_BITRATE_EWMA_FACTOR) +
                                    last_successful_bitrate * MAX_BITRATE_EWMA_FACTOR;
        }
        // Use incoming bitrate only when saturate_bandwidth is on OR if it is within the
        // convergence range
        if (network_settings->saturate_bandwidth ||
            incoming_bitrate * DECREASE_RATIO > max_bitrate_available * CONVERGENCE_THRESHOLD_LOW) {
            new_bitrate = incoming_bitrate * DECREASE_RATIO;
            // If we are reaching convergence than reduce the increase percentage and switch off
            // saturate bandwidth
            if (new_bitrate >= max_bitrate_available * CONVERGENCE_THRESHOLD_LOW) {
                network_settings->saturate_bandwidth = false;
                increase_percentage = max(increase_percentage / 2.0, MIN_INCREASE_PERCENTAGE);
            }
        } else {
            // When saturate bandwidth is OFF, then incoming_bitrate is not reliable. So just reduce
            // the bitrate based on current bitrate and turn on saturate bandwidth so that new
            // bandwidth limit can be found
            // (0.5 * packet_loss_ratio) factor is used as suggested by Google Congestion control
            new_bitrate = network_settings->video_bitrate *
                          min(DECREASE_RATIO, 1.0 - (0.5 * packet_loss_ratio));
            network_settings->saturate_bandwidth = true;
        }
        start_timer(&last_decrease_timer);
    }
    if (new_bitrate != network_settings->video_bitrate) {
        // Till we reach CONVERGENCE_THRESHOLD_LOW of max bitrate in session, bitrate
        // increases will be aggressive
        if (new_bitrate < max_bitrate_available * CONVERGENCE_THRESHOLD_LOW ||
            max_bitrate_available == 0) {
            network_settings->saturate_bandwidth = true;
            increase_percentage = MAX_INCREASE_PERCENTAGE;
        }
        start_timer(&last_update_timer);
        int min_bitrate = MINIMUM_BITRATE;
        if (new_bitrate < min_bitrate) {
            LOG_WARNING("Requested bitrate %d bps is lesser than minimum acceptable bitrate %d bps",
                        new_bitrate, min_bitrate);
            new_bitrate = min_bitrate;
            // If we have reached the min_bitrate for two consecutive times, then signal
            // insufficient bandwidth
            if (network_settings->video_bitrate == min_bitrate) {
                insufficient_bandwidth = true;
            }
        } else {
            insufficient_bandwidth = false;
        }
        burst_mode = false;

        if (new_bitrate >= max_bitrate) {
            network_settings->saturate_bandwidth = false;
            LOG_INFO("Reached the maximum bitrate limit : %d bps", max_bitrate);
            new_bitrate = max_bitrate;
            // More bandwidth than max_bitrate could be available. Switch to burst mode for reduced
            // latency
            burst_mode = true;
        }

        int burst_bitrate = new_bitrate;
        if (burst_mode) burst_bitrate *= BURST_BITRATE_RATIO;
        network_settings->burst_bitrate = burst_bitrate;
        network_settings->video_bitrate = new_bitrate;
        LOG_INFO(
            "New bitrate = %d, burst_bitrate = %d, saturate bandwidth = %d, "
            "max_bitrate_available = %d",
            network_settings->video_bitrate, network_settings->burst_bitrate,
            network_settings->saturate_bandwidth, max_bitrate_available);
        return true;
    } else if (network_settings->saturate_bandwidth && get_timer(&last_update_timer) > 5.0) {
        // Prevent being stuck in saturate_bandwidth loop, without any bitrate update. This can
        // happen when the network bandwidth on this session worsens lesser than
        // (max_bitrate_available  * CONVERGENCE_THRESHOLD_LOW) for a long period of time. In such
        // cases we don't want to be in an infinite quest to reach the maximum bitrate found
        // earlier.
        LOG_INFO("Switch off saturate bandwidth");
        network_settings->saturate_bandwidth = false;
        return true;
    }

    return false;
}
