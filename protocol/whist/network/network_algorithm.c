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

/*
============================
Defines
============================
*/

static NetworkSettings default_network_settings = {
    .bitrate = STARTING_BITRATE,
    .burst_bitrate = STARTING_BURST_BITRATE,
    .desired_codec = CODEC_TYPE_H264,
    .audio_fec_ratio = AUDIO_FEC_RATIO,
    .video_fec_ratio = VIDEO_FEC_RATIO,
    .fps = 60,
};

#define BAD_BITRATE 10400000
#define BAD_BURST_BITRATE 31800000
#define EWMA_STATS_SECONDS 5

/*
============================
Private Function Declarations
============================
*/

// These functions involve various potential
// implementations of `calculate_new_bitrate`

NetworkSettings fallback_bitrate(NetworkStatistics stats);
NetworkSettings ewma_bitrate(NetworkStatistics stats);
NetworkSettings ewma_ratio_bitrate(NetworkStatistics stats);
NetworkSettings timed_ewma_ratio_bitrate(NetworkStatistics stats);
NetworkSettings gcc_bitrate(NetworkStatistics stats);

/*
============================
Public Function Implementations
============================
*/

NetworkSettings get_desired_network_settings(NetworkStatistics stats) {
    // If there are no statistics stored, just return the default network settings
    if (!stats.statistics_gathered) {
        return default_network_settings;
    }
    NetworkSettings network_settings = gcc_bitrate(stats);
    network_settings.fps = default_network_settings.fps;
    network_settings.audio_fec_ratio = default_network_settings.audio_fec_ratio;
    network_settings.video_fec_ratio = default_network_settings.video_fec_ratio;
    network_settings.desired_codec = default_network_settings.desired_codec;
    return network_settings;
}

/*
============================
Private Function Implementations
============================
*/

NetworkSettings fallback_bitrate(NetworkStatistics stats) {
    /*
        Switches between two sets of bitrate/burst bitrate: the default of 16mbps/100mbps and a
        fallback of 10mbps/30mbps. We fall back if we've nacked a lot in the last second.
    */
    static NetworkSettings network_settings;
    if (stats.num_nacks_per_second > 6) {
        network_settings.bitrate = BAD_BITRATE;
        network_settings.burst_bitrate = BAD_BURST_BITRATE;
    } else {
        network_settings.bitrate = STARTING_BITRATE;
        network_settings.burst_bitrate = STARTING_BURST_BITRATE;
    }
    return network_settings;
}

NetworkSettings ewma_bitrate(NetworkStatistics stats) {
    /*
        Keeps an exponentially weighted moving average of the throughput per second the client is
        getting, and uses that to predict a good bitrate to ask the server for.
    */

    // Constants
    const double alpha = 0.8;
    const double bitrate_throughput_ratio = 1.25;

    FATAL_ASSERT(stats.throughput_per_second >= 0);

    // Calculate throughput
    static int throughput = -1;
    if (throughput == -1) {
        throughput = (int)(STARTING_BITRATE / bitrate_throughput_ratio);
    }
    throughput = (int)(alpha * throughput + (1 - alpha) * stats.throughput_per_second);

    // Set network settings
    NetworkSettings network_settings = default_network_settings;
    network_settings.bitrate = (int)(bitrate_throughput_ratio * throughput);
    network_settings.burst_bitrate = STARTING_BURST_BITRATE;
    return network_settings;
}

NetworkSettings timed_ewma_ratio_bitrate(NetworkStatistics stats) {
    /* This is a stopgap to account for a recent change that calls the ewma_ratio_bitrate
       at a frequency for which it was not designed for. This should eventually be replaced
       This updates the ewma_ratio_bitrate steps every 5 seconds as it originally was
       (in sync with the statistics_timer)
       */

    static WhistTimer interval_timer;
    static bool timer_initialized = false;
    static NetworkSettings network_settings = {.bitrate = STARTING_BITRATE,
                                               .burst_bitrate = STARTING_BURST_BITRATE};

    if (!timer_initialized) {
        start_timer(&interval_timer);
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

NetworkSettings gcc_bitrate(NetworkStatistics stats) {
    /*
        Calculate bitrate based on google congestion protocol
        This does the steps of the the delay-based controller and loss-based controller as described
        per the google congestion protocol.
        Arguments:
            stats (BitrateStatistics): struct containing the throughput per second of the client
    */

    // All constants are reccomended values from the paper
    static WhistTimer gcc_init_clock;
    static WhistTimer gcc_timer_clock;
    static int step = 0;
    static const double system_error_variance = 0.1;
    static double kalman_gain = 0;
    static double measurement_variance = 0.5;
    static double filtered_delay_gradient = 0.0;
    static double var_v_hat = 0;
    static const double chi = 0.01;

    static const double adaptive_threshold_overuse_gain = 0.01;
    static const double adaptive_threshold_underuse_gain = 0.01;
    static double overuse_threshold = .06;

    static const double delay_controller_eta = 1.05;
    static const double delay_controller_alpha = 0.9;

    static int receiver_estimated_maximum_bitrate = MINIMUM_BITRATE * 2.5;
    static double average_decrease_remb = MINIMUM_BITRATE * 2.5;
    static double decrease_remb_variance = MINIMUM_BITRATE * 2.5;
    static int decrease_remb_cnt = 0;

    static const double loss_controller_increase_threshold = 0.1;
    static const double loss_controller_decrease_threshold = 0.02;

    enum {
        OVERUSE_DETECTOR_INCREASE_SIGNAL,
        OVERUSE_DETECTOR_HOLD_SIGNAL,
        OVERUSE_DETECTOR_DECREASE_SIGNAL,
    } overuse_detector_signal;

    static const double burst_multiplier = 2.0;
    static NetworkSettings network_settings = {.bitrate = MINIMUM_BITRATE,
                                               .burst_bitrate = MINIMUM_BURST_BITRATE};

    if (stats.num_delay_measurements == 0) {
        LOG_WARNING("NO MEASUREMENTS FOUND!");
        LOG_INFO("%d %d %d %d", stats.num_nacks_per_second, stats.num_received_packets_per_second,
                 stats.num_skipped_frames_per_second, stats.num_rendered_frames_per_second);
        network_settings.bitrate = MINIMUM_BITRATE;
        network_settings.burst_bitrate = MINIMUM_BURST_BITRATE;
        return network_settings;
    }

    if (!step) {
        start_timer(&gcc_init_clock);
        start_timer(&gcc_timer_clock);
    }
    step++;

    stats.average_delay_gradient *= -1;

    // TODO: Move this to a separate timed_gcc function
    if (get_timer(&gcc_timer_clock) < 1 || get_timer(&gcc_init_clock) < 10) {
        return network_settings;
    }

    LOG_INFO("Measured Variance %f", stats.delay_gradient_variance);
    // Arrival filter- calculated delay gradient while accounting for network jitter (modeled as a
    // constant value process so state update equation is not necessary)
    double residual = stats.average_delay_gradient - filtered_delay_gradient;
    double aggregate_variance = system_error_variance + measurement_variance;
    double variance_alpha =
        pow((1 - chi), 30 / (1000 * max(stats.num_received_packets_per_second, 1)));
    var_v_hat = max(variance_alpha * var_v_hat + (1 - variance_alpha) * residual * residual, 0.5);
    kalman_gain = aggregate_variance / (aggregate_variance + var_v_hat);  // gain update
    LOG_INFO("Kalman Gain: %f", kalman_gain);
    filtered_delay_gradient =
        filtered_delay_gradient + residual * kalman_gain;             // filter state update
    measurement_variance = (1 - kalman_gain) * (aggregate_variance);  // measurement variance update

    LOG_INFO("Measured Delay Gradient %f", stats.average_delay_gradient);
    LOG_INFO("Filtered Delay Gradient %f", filtered_delay_gradient);

    LOG_INFO("Average Client Side Delay %f", stats.average_client_side_delay);
    // Adaptive threshold update for overuse detector
    double threshold_gain = (fabs(filtered_delay_gradient) < overuse_threshold)
                                ? adaptive_threshold_underuse_gain
                                : adaptive_threshold_overuse_gain;
    LOG_INFO("Threshold gain %f", threshold_gain);
    overuse_threshold = overuse_threshold + stats.average_client_side_delay * threshold_gain *
                                                (fabs(filtered_delay_gradient) - overuse_threshold);

    LOG_INFO("Overuse threshold %f", overuse_threshold);

    // Detect over/under use using state machine outlined in paper
    if (overuse_threshold < filtered_delay_gradient) {
        overuse_detector_signal = OVERUSE_DETECTOR_DECREASE_SIGNAL;
        LOG_INFO("Signal DECREASE");
    } else if (-overuse_threshold / 10 > filtered_delay_gradient) {
        overuse_detector_signal = OVERUSE_DETECTOR_INCREASE_SIGNAL;
        LOG_INFO("Signal INCREASE");
    } else {
        overuse_detector_signal = OVERUSE_DETECTOR_HOLD_SIGNAL;
        LOG_INFO("Signal HOLD");
    }

    // Delay-based controller selects based on signal
    if (overuse_detector_signal == OVERUSE_DETECTOR_INCREASE_SIGNAL) {
        double std_dev = sqrt(decrease_remb_variance);
        LOG_INFO("2std_dev %f", std_dev * 1.8);
        LOG_INFO("Average Decrease Remb: %f", average_decrease_remb);
        LOG_INFO("stdiff %f", fabs(average_decrease_remb - receiver_estimated_maximum_bitrate));
        if (fabs(average_decrease_remb - receiver_estimated_maximum_bitrate) >= 1.8 * std_dev) {
            // Multiplicative increase state, reset running averages and multiply bitrate
            LOG_INFO("Multiplicative Increase");
            decrease_remb_variance = 0;
            average_decrease_remb = 0;
            decrease_remb_cnt = 0;
            receiver_estimated_maximum_bitrate *= delay_controller_eta;
        } else {
            LOG_INFO("Additive Increase");
            LOG_INFO("average_one_way_trip_latency: %f", stats.average_one_way_trip_latency);
            int response_time_ms = stats.average_one_way_trip_latency * 2 + 100;
            double additive_alpha = 0.5 * min(MS_IN_SECOND / response_time_ms, 1);
            int bits_per_frame = receiver_estimated_maximum_bitrate / 60.0;
            int packets_per_frame = ceil(bits_per_frame / (MAX_PAYLOAD_SIZE * 8));
            int expected_packet_size_bits = bits_per_frame / packets_per_frame;
            LOG_INFO("Additive Constant: %f", additive_alpha * expected_packet_size_bits);
            receiver_estimated_maximum_bitrate +=
                max(1000, additive_alpha * expected_packet_size_bits);
        }
    } else if (overuse_detector_signal == OVERUSE_DETECTOR_DECREASE_SIGNAL) {
        // Update decrease remb statistics
        double variance_residual =
            (decrease_remb_cnt) ? receiver_estimated_maximum_bitrate - average_decrease_remb : 0;
        decrease_remb_variance = ((decrease_remb_variance * decrease_remb_cnt) +
                                  (variance_residual * variance_residual)) /
                                 (decrease_remb_cnt + 1);
        average_decrease_remb =
            ((average_decrease_remb * decrease_remb_cnt) + receiver_estimated_maximum_bitrate) /
            (decrease_remb_cnt + 1);
        decrease_remb_cnt++;
        LOG_INFO("Average Decrease Remb: %f", average_decrease_remb);
        LOG_INFO("Decrease Remb Variance: %f", decrease_remb_variance);
        receiver_estimated_maximum_bitrate *= delay_controller_alpha;
    }

    LOG_INFO("Delay Bitrate: %f", receiver_estimated_maximum_bitrate / 1024.0 / 1024.0);

    // Loss based controller
    double loss_percentage = (double)stats.num_nacks_per_second /
                             (stats.num_received_packets_per_second +
                              stats.num_nacks_per_second);  // Not exact - approximation

    LOG_INFO("Loss percentage %f", loss_percentage);

    if (loss_percentage > loss_controller_increase_threshold) {
        LOG_INFO("LOSS REDUCTION");
        receiver_estimated_maximum_bitrate *= (1 - 0.5 * loss_percentage);
    } else if (loss_percentage < loss_controller_decrease_threshold) {
        LOG_INFO("LOSS INCREASE");
        receiver_estimated_maximum_bitrate *= 1.02;
    }

    // Clamp bitrate
    receiver_estimated_maximum_bitrate =
        min(max(receiver_estimated_maximum_bitrate, MINIMUM_BITRATE), MAXIMUM_BITRATE);

    network_settings.bitrate = receiver_estimated_maximum_bitrate;
    // Paper suggests constant multiplier > 1 for burst bandwidth
    network_settings.burst_bitrate = receiver_estimated_maximum_bitrate * burst_multiplier;

    LOG_INFO("MBPS BITRATE: %f", receiver_estimated_maximum_bitrate / 1024.0 / 1024.0);

    start_timer(&gcc_timer_clock);

    return network_settings;
}
