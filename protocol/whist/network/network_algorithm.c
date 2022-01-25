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

    if (get_forced_values()->verbose_log) {
        LOG_INFO("[current_rates] bitrate=%d burst_bitrate=%d", network_settings.bitrate,
                 network_settings.burst_bitrate);
    }

    if (get_forced_values()->force_bitrate) {
        network_settings.bitrate = get_forced_values()->force_bitrate;
    }
    if (get_forced_values()->force_burst_bitrate) {
        network_settings.burst_bitrate = get_forced_values()->force_burst_bitrate;
    }
    if (get_forced_values()->force_audio_fec_ratio) {
        network_settings.audio_fec_ratio = get_forced_values()->force_audio_fec_ratio;
    }
    if (get_forced_values()->force_video_fec_ratio) {
        network_settings.video_fec_ratio = get_forced_values()->force_video_fec_ratio;
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
