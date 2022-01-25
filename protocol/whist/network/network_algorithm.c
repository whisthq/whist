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

const NetworkSettings default_network_settings = {
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
#define BITS_IN_BYTE 8
#define GOOG_CC_MINIMUM_BITRATE 5500000

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

/*
============================
Public Function Implementations
============================
*/

NetworkSettings get_desired_network_settings(NetworkStatistics stats) {
    // Get the network settings we want, based on those statistics
    NetworkSettings network_settings = timed_ewma_ratio_bitrate(stats);
    network_settings.fps = default_network_settings.fps;
    network_settings.audio_fec_ratio = default_network_settings.audio_fec_ratio;
    network_settings.video_fec_ratio = default_network_settings.video_fec_ratio;
    network_settings.desired_codec = default_network_settings.desired_codec;

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

    // Return the network settings
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

int goog_cc_loss_controller(NetworkStatistics stats, int receiver_estimated_maximum_bitrate) {
    /* Loss controller from google congestion control spec. Uses the receiver estimated
       maximum bitrate and loss statistics time to calculate a loss bitrate per the spec.
       This (along with sending the bitrate msg) should be called every 1 second.
    */

    static const double loss_controller_increase_threshold = 0.1;
    static const double loss_controller_decrease_threshold = 0.02;

    // Loss based controller
    double loss_percentage = (double)stats.num_nacks_per_second /
                             (stats.num_received_packets_per_second +
                              stats.num_nacks_per_second);  // Approximate lost packets

    if (loss_percentage > loss_controller_increase_threshold) {
        return receiver_estimated_maximum_bitrate * (1 - 0.5 * loss_percentage);
    } else if (loss_percentage < loss_controller_decrease_threshold) {
        return receiver_estimated_maximum_bitrate * 1.05;
    }

    return receiver_estimated_maximum_bitrate;
}

int goog_cc_delay_controller(NetworkStatistics stats) {
    /* Delay controller from google congestion control spec. Uses the delay
       gradient and round trip time to calculate receiver_estimated_maximum_bitrate
       according to the goog_cc spec. Should be called per received frame.
    */

    static WhistTimer overuse_timer;
    static bool delay_controller_initialized = false;

    static const double system_error_variance = 0.1;
    static double kalman_gain = 0;
    static double measurement_variance = 0.5;
    static double filtered_delay_gradient = 0.0;
    static double prev_filtered_delay_gradient = 0.0;
    static double var_v_hat = 0;
    static const double chi = 0.01;

    static const double adaptive_threshold_overuse_gain = 0.01;      // K_u
    static const double adaptive_threshold_underuse_gain = 0.00018;  // K_d
    static const int adaptive_threshold_diff_limit = 15;
    static double overuse_threshold = 12.5;
    static const double overuse_threshold_min = 1;
    static const double overuse_threshold_max = 600;

    static const double delay_controller_eta = 1.05;
    static const double delay_controller_alpha = 0.85;

    static const double overuse_time_th = 0.01;

    static int receiver_estimated_maximum_bitrate = MINIMUM_BITRATE;
    static double average_decrease_remb = 0;
    static double decrease_remb_variance = 0;
    static const double num_std_devs_threshold = 1.8;
    static int decrease_remb_cnt = 0;

    if (!delay_controller_initialized) {
        start_timer(&overuse_timer);
        delay_controller_initialized = true;
    }

    static enum {
        OVERUSE_DETECTOR_UNDERUSE_SIGNAL,
        OVERUSE_DETECTOR_NORMAL_SIGNAL,
        OVERUSE_DETECTOR_OVERUSE_SIGNAL,
    } overuse_detector_signal = OVERUSE_DETECTOR_NORMAL_SIGNAL;

    static enum {
        DELAY_CONTROLLER_INCREASE,
        DELAY_CONTROLLER_HOLD,
        DELAY_CONTROLLER_DECREASE
    } delay_controller_state = DELAY_CONTROLLER_HOLD;

    // Arrival filter
    double residual = stats.delay_gradient - filtered_delay_gradient;
    double aggregate_variance = system_error_variance + measurement_variance;
    double variance_alpha =
        pow((1 - chi), 30 / (1000 * max(stats.num_received_packets_per_second, 1)));
    var_v_hat = max(variance_alpha * var_v_hat + (1 - variance_alpha) * residual * residual, 0.5);
    kalman_gain = aggregate_variance / (aggregate_variance + var_v_hat);  // gain update
    filtered_delay_gradient =
        filtered_delay_gradient + residual * kalman_gain;             // filter state update
    measurement_variance = (1 - kalman_gain) * (aggregate_variance);  // measurement variance update

    // Adaptive threshold update for overuse detector
    if ((fabs(filtered_delay_gradient) - overuse_threshold) < adaptive_threshold_diff_limit) {
        double threshold_gain = (fabs(filtered_delay_gradient) < overuse_threshold)
                                    ? adaptive_threshold_underuse_gain
                                    : adaptive_threshold_overuse_gain;
        overuse_threshold =
            overuse_threshold + stats.client_side_delay * threshold_gain *
                                    (fabs(filtered_delay_gradient) - overuse_threshold);
        // Clamp dynamic threshold to reccomended range by spec
        overuse_threshold =
            min(max(overuse_threshold, overuse_threshold_min), overuse_threshold_max);
    }

    // Detect over/under use using state machine outlined in spec
    if (overuse_threshold < filtered_delay_gradient) {
        if (get_timer(&overuse_timer) > overuse_time_th &&
            filtered_delay_gradient >= prev_filtered_delay_gradient) {
            // Signal clear overuse
            overuse_detector_signal = OVERUSE_DETECTOR_OVERUSE_SIGNAL;
            start_timer(&overuse_timer);
        }
    } else if (-overuse_threshold > filtered_delay_gradient) {
        overuse_detector_signal = OVERUSE_DETECTOR_UNDERUSE_SIGNAL;
    } else {
        overuse_detector_signal = OVERUSE_DETECTOR_NORMAL_SIGNAL;
    }

    // update delay-based controller state
    if (overuse_detector_signal == OVERUSE_DETECTOR_OVERUSE_SIGNAL) {
        delay_controller_state = DELAY_CONTROLLER_DECREASE;
    } else if (overuse_detector_signal == OVERUSE_DETECTOR_NORMAL_SIGNAL) {
        if (delay_controller_state == DELAY_CONTROLLER_HOLD) {
            delay_controller_state = DELAY_CONTROLLER_INCREASE;
        } else if (delay_controller_state == DELAY_CONTROLLER_DECREASE) {
            delay_controller_state = DELAY_CONTROLLER_HOLD;
        }
    } else if (overuse_detector_signal == OVERUSE_DETECTOR_UNDERUSE_SIGNAL) {
        // Double checked this - underuse always translates to hold
        delay_controller_state = DELAY_CONTROLLER_HOLD;
    }

    // Delay-based controller selects based on overuse signal
    if (delay_controller_state == DELAY_CONTROLLER_INCREASE) {
        // Calculate standard deviation of REMB from decrease state
        double std_dev = sqrt(decrease_remb_variance);
        int incoming_bitrate =
            stats.num_received_packets_per_second * BITS_IN_BYTE * MAX_PAYLOAD_SIZE;
        if (fabs(average_decrease_remb - incoming_bitrate) >= num_std_devs_threshold * std_dev) {
            // Multiplicative increase state, reset running averages and multiply bitrate
            decrease_remb_variance = 0;
            average_decrease_remb = 0;
            decrease_remb_cnt = 0;
            receiver_estimated_maximum_bitrate *= delay_controller_eta;
        } else {
            // Additive increase use estimated packet size for linear increase in bitrate
            int response_time_ms = stats.one_way_trip_time * 2 + 100;
            // Estimate RTT has 2 * one way trip time
            double additive_alpha = 0.5 * min(MS_IN_SECOND / response_time_ms, 1);
            int bits_per_frame = receiver_estimated_maximum_bitrate / 60.0;
            int packets_per_frame = ceil(bits_per_frame / (MAX_PAYLOAD_SIZE * 8));
            int expected_packet_size_bits = bits_per_frame / packets_per_frame;
            receiver_estimated_maximum_bitrate +=
                max(1000, additive_alpha * expected_packet_size_bits);
        }
    } else if (delay_controller_state == DELAY_CONTROLLER_DECREASE) {
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
        // Decay delay-based bitrate
        receiver_estimated_maximum_bitrate *= delay_controller_alpha;
    }

    // Store for use with next overuse signal condition
    prev_filtered_delay_gradient = filtered_delay_gradient;

    return receiver_estimated_maximum_bitrate;
}

NetworkSettings goog_cc_bitrate(NetworkStatistics stats) {
    /*
        Calculate bitrate based on google congestion protocol
        This does the steps of the the delay-based controller and loss-based controller as described
        per the google congestion protocol. However, instead of sending an RTCP packet every second
        to the server (after the delay controller) we just implement the loss controller here and
        rate limit it with a timer. The output of the loss controller is returned as a new value.
        Arguments:
            stats (BitrateStatistics): struct containing the throughput per second of the client
                - delay_gradient: This is the queuing delay gradient as described by the spec
                - goog_cc_ready: Since we do not have complete control over when the feedback is
                  called this flag should be set true when a packet is received with enough
                  information to make the proper gradient delay measurements
                - one_way_trip: Latency between server/client used to approximate round trip time
                - client_side_delay: The arrival difference between the current group of packets in
                  the complete frame and the previous group
                - nacked_packets and received_packets are used to approximate loss percentage
                - other reception/throughput values are also used for computing volume stats
    */
    static bool goog_cc_initialized = false;
    static const double delay_remb_bound_multiplier = 1.5;

    static WhistTimer goog_cc_loss_timer;
    static NetworkSettings network_settings = {.bitrate = GOOG_CC_MINIMUM_BITRATE,
                                               .burst_bitrate = MINIMUM_BURST_BITRATE};

    // Start timers and return starting bitrate on first run
    if (!goog_cc_initialized) {
        start_timer(&goog_cc_loss_timer);
        goog_cc_initialized = true;
        return network_settings;
    }

    // Prevent goog_cc delay controller from when new frame has not been received
    if (!stats.goog_cc_ready) {
        return network_settings;
    }

    int receiver_estimated_maximum_bitrate = goog_cc_delay_controller(stats);

    // Delay controller should run every received frame, but loss controller is limited to every 1
    // second unless there is congestion detected by the delay controller
    if (get_timer(&goog_cc_loss_timer) < 1 &&
        receiver_estimated_maximum_bitrate >= network_settings.bitrate) {
        return network_settings;
    }

    // Don't let delay-based bitrate estimate stray too far from current bitrate per spec
    if (receiver_estimated_maximum_bitrate >
        network_settings.bitrate * delay_remb_bound_multiplier) {
        receiver_estimated_maximum_bitrate = network_settings.bitrate * delay_remb_bound_multiplier;
    }

    int loss_controller_bitrate =
        goog_cc_loss_controller(stats, receiver_estimated_maximum_bitrate);
    int target_bitrate = min(receiver_estimated_maximum_bitrate, loss_controller_bitrate);

    // Clamp and set bitrate for new network settings
    target_bitrate = min(max(target_bitrate, GOOG_CC_MINIMUM_BITRATE), MAXIMUM_BITRATE);
    network_settings.bitrate = target_bitrate;
    network_settings.burst_bitrate = target_bitrate;
    start_timer(&goog_cc_loss_timer);

    return network_settings;
}
