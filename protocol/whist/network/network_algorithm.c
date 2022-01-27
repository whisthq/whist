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
#include "whist/tools/debug_console.h"

/*
============================
Globals
============================
*/

volatile int output_width;
volatile int output_height;

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
    .saturate_bandwidth = true,
};

#define DPI_BITRATE_PER_PIXEL 192

#define EWMA_STATS_SECONDS 5

// Confirmed visually that these values produce reasonable output quality for DPI of 192. For other
// DPIs they need to scaled accordingly.
#define MINIMUM_BITRATE_PER_PIXEL 1.0
#define MAXIMUM_BITRATE_PER_PIXEL 4.0
#define STARTING_BITRATE_PER_PIXEL 3.0

// This value directly impacts the time to transmit big frames. Also this value indirectly impacts
// the video quality as the VBV buffer size is based on this ratio. Also if this value is set too
// high then we will observe packet loss. So it is tradeoff between packet loss vs latency/quality
#define BURST_BITRATE_RATIO 4

#define TOTAL_AUDIO_BITRATE ((NUM_PREV_AUDIO_FRAMES_RESEND + 1) * AUDIO_BITRATE)

// This value was decided heuristically by visually comparing similar resolution windows in 192 DPI
// and 96 DPI screens. This translates to 96 DPI screens having a bitrate ~3x more than 192 DPI
// screen for same resolution.
#define DPI_RATIO_EXPONENT 1.6

#define MINIMUM_BITRATE \
    get_total_bitrate(output_width, output_height, dpi, MINIMUM_BITRATE_PER_PIXEL)
#define MAXIMUM_BITRATE \
    get_total_bitrate(output_width, output_height, dpi, MAXIMUM_BITRATE_PER_PIXEL)
#define STARTING_BITRATE \
    get_total_bitrate(output_width, output_height, dpi, STARTING_BITRATE_PER_PIXEL)

#define MINIMUM_BURST_BITRATE (MINIMUM_BITRATE * BURST_BITRATE_RATIO)
#define MAXIMUM_BURST_BITRATE (MAXIMUM_BITRATE * BURST_BITRATE_RATIO)
#define STARTING_BURST_BITRATE (STARTING_BITRATE * BURST_BITRATE_RATIO)

static int dpi = -1;

/*
============================
Private Function Declarations
============================
*/

// These functions involve various potential
// implementations of `calculate_new_bitrate`

NetworkSettings ewma_ratio_bitrate(NetworkStatistics stats);
NetworkSettings timed_ewma_ratio_bitrate(NetworkStatistics stats);

static int get_video_bitrate(int width, int height, int screen_dpi, double bitrate_per_pixel) {
    // We have to scale-up the bitrates for lower DPI screens. This is because Chrome renders more
    // content in lower DPI screens and hence higher level of details. So the same bitrate cannot be
    // used for screens of different DPIs
    // Also limit the DPI ratio, so that we don't get crazy values
    double dpi_ratio = max(min((double)DPI_BITRATE_PER_PIXEL / screen_dpi, 2.0), 0.5);
    double dpi_scaling_factor = pow(dpi_ratio, DPI_RATIO_EXPONENT);
    return (int)(width * height * bitrate_per_pixel * dpi_scaling_factor);
}

static int get_total_bitrate(int width, int height, int screen_dpi, double bitrate_per_pixel) {
    return get_video_bitrate(width, height, screen_dpi, bitrate_per_pixel) + TOTAL_AUDIO_BITRATE;
}

/*
============================
Public Function Implementations
============================
*/

NetworkSettings get_default_network_settings(int width, int height, int screen_dpi) {
    FATAL_ASSERT(width > 0 && height > 0 && screen_dpi > 0);
    default_network_settings.bitrate =
        get_total_bitrate(width, height, screen_dpi, STARTING_BITRATE_PER_PIXEL);
    default_network_settings.burst_bitrate = default_network_settings.bitrate * BURST_BITRATE_RATIO;
    return default_network_settings;
}

NetworkSettings get_starting_network_settings(void) {
    return get_default_network_settings(output_width, output_height, dpi);
}

NetworkSettings get_desired_network_settings(NetworkStatistics stats) {
    // Get the network settings we want, based on those statistics
    NetworkSettings network_settings = timed_ewma_ratio_bitrate(stats);
    network_settings.fps = default_network_settings.fps;
    network_settings.audio_fec_ratio = default_network_settings.audio_fec_ratio;
    network_settings.video_fec_ratio = default_network_settings.video_fec_ratio;
    network_settings.desired_codec = default_network_settings.desired_codec;

    if (get_debug_console_override_values()->verbose_log) {
        LOG_INFO("[current_rates] bitrate=%d burst_bitrate=%d", network_settings.bitrate,
                 network_settings.burst_bitrate);
    }

    if (get_debug_console_override_values()->bitrate) {
        network_settings.bitrate = get_debug_console_override_values()->bitrate;
    }
    if (get_debug_console_override_values()->burst_bitrate) {
        network_settings.burst_bitrate = get_debug_console_override_values()->burst_bitrate;
    }
    if (get_debug_console_override_values()->audio_fec_ratio) {
        network_settings.audio_fec_ratio = get_debug_console_override_values()->audio_fec_ratio;
    }
    if (get_debug_console_override_values()->video_fec_ratio) {
        network_settings.video_fec_ratio = get_debug_console_override_values()->video_fec_ratio;
    }

    // Clamp to bounds
    if (network_settings.bitrate < MINIMUM_BITRATE) {
        network_settings.bitrate = MINIMUM_BITRATE;
    }
    if (network_settings.bitrate > MAXIMUM_BITRATE) {
        network_settings.bitrate = MAXIMUM_BITRATE;
    }
    if (network_settings.burst_bitrate < MINIMUM_BURST_BITRATE) {
        network_settings.burst_bitrate = MINIMUM_BURST_BITRATE;
    }
    if (network_settings.burst_bitrate > MAXIMUM_BURST_BITRATE) {
        network_settings.burst_bitrate = MAXIMUM_BURST_BITRATE;
    }

    // Clamp burst bitrate to within ratio of avg bitrate
    network_settings.burst_bitrate =
        min(network_settings.burst_bitrate, network_settings.bitrate * BURST_BITRATE_RATIO);

    // Return the network settings
    return network_settings;
}

void network_algo_set_dpi(int new_dpi) { dpi = new_dpi; }

/*
============================
Private Function Implementations
============================
*/

NetworkSettings timed_ewma_ratio_bitrate(NetworkStatistics stats) {
    /* This is a stopgap to account for a recent change that calls the ewma_ratio_bitrate
       at a frequency for which it was not designed for. This should eventually be replaced
       This updates the ewma_ratio_bitrate steps every 5 seconds as it originally was
       (in sync with the statistics_timer)
       */

    static WhistTimer interval_timer;
    static bool timer_initialized = false;
    static NetworkSettings network_settings;

    if (!timer_initialized) {
        start_timer(&interval_timer);
        network_settings = get_starting_network_settings();
        timer_initialized = true;
    }

    if (get_timer(&interval_timer) > EWMA_STATS_SECONDS) {
        network_settings = ewma_ratio_bitrate(stats);
        start_timer(&interval_timer);
    }

    return network_settings;
}

NetworkSettings ewma_ratio_bitrate(NetworkStatistics stats) {
    /*
        Keeps an exponentially weighted moving average of the throughput per second the client is
        getting, and uses that to predict a good bitrate to ask the server for. The throughput
        per second that the client is getting is estimated by the ratio of successful packets to
        total packets (successful + NACKed) multiplied by the active throughput. Due to the usage
        of a heuristic for finding current throughput, every `same_throughput_count_threshold`
        periods, we boost the throughput estimate if the calculated throughput has remained the
        same for `same_throughput_count_threshold` periods. If the boosted throughput results in
        NACKs, then we revert to the previous throughput and increase the value of
        `same_throughput_count_threshold` so that the successful throughput stays constant
        for longer and longer periods of time before we try higher network_settings. We follow a
       similar logic flow for the burst bitrate, except we use skipped renders instead of NACKs.
    */
    const double alpha = 0.8;
    // Because the max bitrate of the encoder is usually larger than the actual amount of data we
    //     get from the server
    const double bitrate_throughput_ratio = 1.25;
    // Multiplier when boosting throughput/bitrate after continuous success
    const double boost_multiplier = 1.05;

    // Hacky way of allowing constant assignment to static variable (cannot assign `const` to
    //     `static`)
    // In order:
    //     > the minimum setting for threshold for meeting throughput expectations before boosting
    //     > the multiplier for threshold after successfully meeting throughput expectations
    //     > the maximum setting for threshold for meeting throughput expectations before boosting
    enum {
        meet_expectations_min = 5,         // NOLINT(readability-identifier-naming)
        meet_expectations_multiplier = 2,  // NOLINT(readability-identifier-naming)
        meet_expectations_max = 1024       // NOLINT(readability-identifier-naming)
    };

    // Target throughput (used to calculate bitrate)
    static int expected_throughput = -1;
    // How many times in a row that the real throughput has met expectations
    static int met_throughput_expectations_count = 0;
    // The threshold that the current throughput needs to meet to be boosted
    static int meet_throughput_expectations_threshold = meet_expectations_min;
    // The latest throughput that met expectations up to threshold
    static int latest_successful_throughput = -1;
    // The threshold that the latest successful throughput needs to meet to be boosted
    static int latest_successful_throughput_threshold = meet_expectations_min;

    // How many times in a row that the burst has met expectations
    static int met_burst_expectations_count = 0;
    // The threshold that the current burst needs to meet to be boosted
    static int meet_burst_expectations_threshold = meet_expectations_min;
    // The latest burst that met expectations up to threshold
    static int latest_successful_burst = -1;
    // The threshold that the latest successful burst needs to meet to be boosted
    static int latest_successful_burst_threshold = meet_expectations_min;

    static NetworkSettings network_settings;
    if (expected_throughput == -1) {
        expected_throughput = (int)(STARTING_BITRATE / bitrate_throughput_ratio);
        network_settings.burst_bitrate = STARTING_BURST_BITRATE;
    }

    // Make sure throughput is not negative and also that the client has received frames at all
    //     i.e., we don't want to recalculate bitrate if the video is static and server is sending
    //      zero packets
    if (stats.num_received_packets_per_second + stats.num_nacks_per_second > 0) {
        int real_throughput =
            (int)(expected_throughput * (double)stats.num_received_packets_per_second /
                  (stats.num_nacks_per_second + stats.num_received_packets_per_second));

        if (real_throughput == expected_throughput) {
            // If this throughput meets expectations, then increment the met expectation count
            met_throughput_expectations_count++;

            // If we have met expectations for the current threshold number of iterations,
            //     then set the latest successful throughput to the active throughput and boost the
            //     expected throughput to test a higher bitrate.
            if (met_throughput_expectations_count >= meet_throughput_expectations_threshold) {
                latest_successful_throughput = real_throughput;
                met_throughput_expectations_count = 0;
                latest_successful_throughput_threshold = meet_throughput_expectations_threshold;
                meet_throughput_expectations_threshold = meet_expectations_min;
                expected_throughput = (int)(expected_throughput * boost_multiplier);
            }
        } else {
            if (expected_throughput > latest_successful_throughput &&
                latest_successful_throughput != -1) {
                // If the expected throughput that yielded lost packets was higher than the
                //     last continuously successful throughput, then set the expected
                //     throughput to the last continuously successful throughput to try it
                //     again for longer. Increase the threshold for boosting the expected throughput
                //     after continuous success because it is more likely that we have found
                //     our successful throughput for now.
                expected_throughput = latest_successful_throughput;
                latest_successful_throughput_threshold =
                    min(latest_successful_throughput_threshold * meet_expectations_multiplier,
                        meet_expectations_max);
                meet_throughput_expectations_threshold = latest_successful_throughput_threshold;
            } else {
                // If the expected throughput that yielded lost packets was lower than the last
                //     continuously successful throughput, then set the expected throughput to an
                //     EWMA-calculated new value and reset the threshold for boosting
                //     the expected throughput after meeting threshold success.
                expected_throughput =
                    (int)(alpha * expected_throughput + (1 - alpha) * real_throughput);
                meet_throughput_expectations_threshold = meet_expectations_min;
            }

            met_throughput_expectations_count = 0;
        }

        network_settings.bitrate = (int)(bitrate_throughput_ratio * expected_throughput);

        if (network_settings.bitrate > MAXIMUM_BITRATE) {
            network_settings.bitrate = MAXIMUM_BITRATE;
            expected_throughput = (int)(MAXIMUM_BITRATE / bitrate_throughput_ratio);
        } else if (network_settings.bitrate < MINIMUM_BITRATE) {
            network_settings.bitrate = MINIMUM_BITRATE;
            expected_throughput = (int)(MINIMUM_BITRATE / bitrate_throughput_ratio);
        }
    }

    // Make sure that frames have been recieved before trying to update the burst bitrate
    if (stats.num_rendered_frames_per_second > 0) {
        int current_burst_heuristic =
            (int)(network_settings.burst_bitrate * (double)stats.num_rendered_frames_per_second /
                  stats.num_rendered_frames_per_second);

        if (current_burst_heuristic == network_settings.burst_bitrate) {
            // If this burst bitrate is the same as the burst bitrate from the last period, then
            //     increment the same burst bitrate count
            met_burst_expectations_count++;

            // If we have had the same burst bitrate for the current threshold number of iterations,
            //     then set the latest successful burst bitrate to the active burst bitrate and
            //     boost the burst bitrate to test a higher burst bitrate. Reset the threshold for
            //     boosting the burst bitrate after continuous success since we may have entered an
            //     area with higher bandwidth.
            if (met_burst_expectations_count >= meet_burst_expectations_threshold) {
                latest_successful_burst = network_settings.burst_bitrate;
                met_burst_expectations_count = 0;
                latest_successful_burst_threshold = meet_throughput_expectations_threshold;
                meet_burst_expectations_threshold = meet_expectations_min;
                network_settings.burst_bitrate =
                    (int)(network_settings.burst_bitrate * boost_multiplier);
            }
        } else {
            if (network_settings.burst_bitrate > latest_successful_burst &&
                latest_successful_burst != -1) {
                // If the burst bitrate that yielded lost packets was higher than the last
                //     continuously successful burst bitrate, then set the burst bitrate to
                //     the last continuously successful burst bitrate to try it again for longer.
                //     Increase the threshold for boosting the burst bitrate after continuous
                //     success because it is more likely that we have found our successful
                //     burst bitrate for now.
                network_settings.burst_bitrate = latest_successful_burst;
                latest_successful_burst_threshold =
                    min(latest_successful_burst_threshold * meet_expectations_multiplier,
                        meet_expectations_max);
                meet_burst_expectations_threshold = latest_successful_burst_threshold;
            } else {
                // If the burst bitrate that yielded lost packets was lower than the last
                //     continuously successful burst bitrate, then set the burst bitrate to an
                //     EWMA-calculated new value and reset the threshold for boosting
                //     the burst bitrate after continuous success.
                network_settings.burst_bitrate = (int)(alpha * network_settings.burst_bitrate +
                                                       (1 - alpha) * current_burst_heuristic);
                meet_burst_expectations_threshold = meet_expectations_min;
            }

            met_burst_expectations_count = 0;
        }

        if (network_settings.burst_bitrate > STARTING_BURST_BITRATE) {
            network_settings.burst_bitrate = STARTING_BURST_BITRATE;
        } else if (network_settings.burst_bitrate < MINIMUM_BITRATE) {
            network_settings.burst_bitrate = MINIMUM_BITRATE;
        }
    }
    return network_settings;
}

// The theory behind all the code in this function is documented in WCC.md file. Please go thru that
// document before reviewing this file. Also if you make any modifications to this algo, remember to
// update the WCC.md file as well so that the documentation remains upto date.
bool whist_congestion_controller(GroupStats *curr_group_stats, GroupStats *prev_group_stats,
                                 int incoming_bitrate, double packet_loss_ratio,
                                 NetworkSettings *network_settings) {
#define INITIAL_PRE_BURST_MODE_COUNT 5.0
#define INITIAL_INCREASE_PERCENTAGE 1.0
// Latest delay variation gets this weightage. Older value gets a weightage of (1 - EWMA_FACTOR)
#define EWMA_FACTOR 0.3
#define DELAY_VARIATION_THRESHOLD_IN_SEC 0.01  // 10ms
#define OVERUSE_TIME_THRESHOLD_IN_SEC 0.01     // 10ms
#define NEW_BITRATE_DURATION_IN_SEC 1.0
#define CONVERGENCE_THRESHOLD_LOW 0.8
#define CONVERGENCE_THRESHOLD_HIGH 1.1
#define DECREASE_RATIO 0.95
#define BANDWITH_USED_THRESHOLD 0.9

    static WhistTimer overuse_timer;
    static WhistTimer last_update_timer;
    static WhistTimer last_decrease_timer;
    static bool delay_controller_initialized = false;

    static double filtered_delay_variation;
    static double increase_percentage = INITIAL_INCREASE_PERCENTAGE;
    static bool burst_mode = false;
    static double pre_burst_mode_count = INITIAL_PRE_BURST_MODE_COUNT;
    static int last_increase_bitrate;
    static int last_failed_bitrate;
    static int max_bitrate_count = 0;

    if (!delay_controller_initialized) {
        start_timer(&overuse_timer);
        start_timer(&last_update_timer);
        start_timer(&last_decrease_timer);
        // Adjust the last decrease timer, to respond immediately for congestion during startup.
        adjust_timer(&last_decrease_timer, (int)-NEW_BITRATE_DURATION_IN_SEC);
        last_failed_bitrate = last_increase_bitrate = network_settings->bitrate;
    }
    int max_bitrate = MAXIMUM_BITRATE;
    int new_bitrate = network_settings->bitrate;

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
    if (DELAY_VARIATION_THRESHOLD_IN_SEC < filtered_delay_variation) {
        if (!maybe_overuse) {
            start_timer(&overuse_timer);
            maybe_overuse = true;
        }
        // A definitive over-use will be signaled only if over-use has been
        // detected for at least overuse_time_th milliseconds.  However, if m(i)
        // < m(i-1), over-use will not be signaled even if all the above
        // conditions are met.
        if (get_timer(&overuse_timer) > OVERUSE_TIME_THRESHOLD_IN_SEC /* &&
            filtered_delay_variation >= prev_filtered_delay_variation*/) {
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

    if (packet_loss_ratio > 0.1) {
        overuse_detector_signal = OVERUSE_SIGNAL;
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

    // Delay-based controller selects based on overuse signal
    // It is RECOMMENDED to send the REMB message as soon
    // as congestion is detected, and otherwise at least once every second.
    // It is RECOMMENDED that the routine to update A_hat(i) is run at least
    // once every response_time interval.
    if (delay_controller_state == DELAY_CONTROLLER_INCREASE &&
        get_timer(&last_update_timer) > NEW_BITRATE_DURATION_IN_SEC &&
        incoming_bitrate > (network_settings->bitrate * BANDWITH_USED_THRESHOLD)) {
        LOG_INFO("Increase bitrate by %.2f percent", increase_percentage);
        new_bitrate = network_settings->bitrate * (1.0 + increase_percentage / 100.0);
        last_increase_bitrate = new_bitrate;
        // Looks like the network has recovered. Reset the counters.
        if (new_bitrate > last_failed_bitrate * CONVERGENCE_THRESHOLD_HIGH) {
            network_settings->saturate_bandwidth = true;
            increase_percentage = INITIAL_INCREASE_PERCENTAGE;
        }

    } else if ((delay_controller_state == DELAY_CONTROLLER_DECREASE) &&
               get_timer(&last_decrease_timer) > NEW_BITRATE_DURATION_IN_SEC) {
        LOG_INFO("Decrease bitrate filtered_delay_variation = %.2f packet_loss_ratio = %.2f",
                 filtered_delay_variation, packet_loss_ratio);
        new_bitrate = incoming_bitrate * DECREASE_RATIO;
        start_timer(&last_decrease_timer);
        if (burst_mode && max_bitrate_count < 2 * pre_burst_mode_count) {
            // If the burst mode couldn't sustain pre_burst_mode_count, then the current congestion
            // could be because of burst mode. So we make it difficult to get into burst mode.
            pre_burst_mode_count *= 2;
        } else if (network_settings->bitrate < max_bitrate) {
            // If congestion is detected even at bitrates lesser than max_bitrate then reset the
            // burst mode related thresholds to its original values.
            pre_burst_mode_count = INITIAL_PRE_BURST_MODE_COUNT;
        }
        if (new_bitrate < last_increase_bitrate * CONVERGENCE_THRESHOLD_LOW) {
            network_settings->saturate_bandwidth = true;
            increase_percentage = INITIAL_INCREASE_PERCENTAGE;
        } else {
            network_settings->saturate_bandwidth = false;
            increase_percentage /= 2.0;
        }
        last_failed_bitrate = network_settings->bitrate;
    }

    if (new_bitrate != network_settings->bitrate) {
        start_timer(&last_update_timer);
        int min_bitrate = MINIMUM_BITRATE;
        if (new_bitrate < min_bitrate) {
            LOG_WARNING("Requested bitrate %d bps is lesser than minimum acceptable bitrate %d bps",
                        new_bitrate, min_bitrate);
            new_bitrate = min_bitrate;
        }
        if (new_bitrate > max_bitrate) {
            network_settings->saturate_bandwidth = false;
            LOG_INFO("Reached the maximum bitrate limit : %d bps", max_bitrate);
            if (max_bitrate_count >= pre_burst_mode_count) {
                if (!burst_mode) {
                    LOG_INFO("Switching to burst mode to reduce latency");
                    burst_mode = true;
                }
            }
            new_bitrate = max_bitrate;
            max_bitrate_count++;
        } else {
            burst_mode = false;
            max_bitrate_count = 0;
        }
        int burst_bitrate = new_bitrate;
        if (burst_mode) burst_bitrate *= BURST_BITRATE_RATIO;
        network_settings->burst_bitrate = burst_bitrate;
        network_settings->bitrate = new_bitrate;
        LOG_INFO("New bitrate = %d, burst_bitrate = %d, saturate bandwidth = %d",
                 network_settings->bitrate, network_settings->burst_bitrate,
                 network_settings->saturate_bandwidth);
        return true;
    }

    return false;
}
