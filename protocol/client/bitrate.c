/**
 * Copyright Fractal Computers, Inc. 2021
 * @file bitrate.h
 * @brief This file contains the client's adaptive bitrate code. Any algorithms we are using for
predicting bitrate should be stored here.
============================
Usage
============================
Place to put any predictive/adaptive bitrate algorithms. In the current setup, each algorithm is a
function that takes in inputs through a BitrateStatistics struct. The function is responsible for
maintaining any internal state necessary for the algorithm (possibly through the use of custom types
or helpers), and should update max_bitrate when necessary. To change the algorithm the client uses,
set calculate_new_bitrate to the desired algorithm.
*/

/*
============================
Includes
============================
*/
#include "bitrate.h"

volatile int max_bitrate = STARTING_BITRATE;
volatile int max_burst_bitrate = STARTING_BURST_BITRATE;
#define BAD_BITRATE 10400000
#define BAD_BURST_BITRATE 31800000

/*
============================
Private Functions
============================
*/
Bitrates fallback_bitrate(BitrateStatistics stats);
Bitrates ewma_bitrate(BitrateStatistics stats);

/*
============================
Private Function Implementations
============================
*/
Bitrates fallback_bitrate(BitrateStatistics stats) {
    /*
        Switches between two sets of bitrate/burst bitrate: the default of 16mbps/100mbps and a
       fallback of 10mbps/30mbps. We fall back if we've nacked a lot in the last second.

        Arguments:
            stats (BitrateStatistics): struct containing the average number of nacks per second
       since the last time this function was called
    */
    static Bitrates bitrates;
    if (stats.num_nacks_per_second > 6 && max_bitrate != BAD_BITRATE) {
        bitrates.bitrate = BAD_BITRATE;
        bitrates.burst_bitrate = BAD_BURST_BITRATE;
    } else {
        bitrates.bitrate = max_bitrate;
        bitrates.burst_bitrate = max_burst_bitrate;
    }
    return bitrates;
}

Bitrates ewma_bitrate(BitrateStatistics stats) {
    /*
        Keeps an exponentially weighted moving average of the throughput per second the client is
        getting, and uses that to predict a good bitrate to ask the server for.

        Arguments:
            stats (BitrateStatistics): struct containing the throughput per second of the client
    */
    static const double alpha = 0.8;
    // because the max bitrate of the encoder is usually larger than the actual amount of data we
    // get from the server
    static const double bitrate_throughput_ratio = 1.25;
    static int throughput = -1;
    static Bitrates bitrates;
    if (throughput == -1) {
        throughput = (int)(STARTING_BITRATE / bitrate_throughput_ratio);
    }
    // sanity check to make sure we're not sending it negative bitrate
    if (stats.throughput_per_second >= 0) {
        throughput = (int)(alpha * throughput + (1 - alpha) * stats.throughput_per_second);
        bitrates.bitrate = (int)(bitrate_throughput_ratio * throughput);
    }
    bitrates.burst_bitrate = max_burst_bitrate;
    return bitrates;
}

Bitrates ewma_ratio_bitrate(BitrateStatistics stats) {
    /*
        Keeps an exponentially weighted moving average of the throughput per second the client is
        getting, and uses that to predict a good bitrate to ask the server for. The throughput
        per second that the client is getting is estimated by the ratio of successful packets to
        total packets (successful + NACKed) multiplied by the active throughput.

        Arguments:
            stats (BitrateStatistics): struct containing the throughput per second of the client
    */
    static const double alpha = 0.8;
    // because the max bitrate of the encoder is usually larger than the actual amount of data we
    // get from the server
    static const double bitrate_throughput_ratio = 1.25;

    // hacky way of allowing constant assignment to static variable (cannot assign `const` to
    //     `static)
    enum {
        same_count_min = 5,         // NOLINT(readability-identifier-naming)
        same_count_multiplier = 2,  // NOLINT(readability-identifier-naming)
        same_count_max = 1024       // NOLINT(readability-identifier-naming)
    };

    static int throughput = -1;
    static int same_throughput_count = 0;
    static int max_successful_throughput = -1;
    static int same_throughput_count_threshold = same_count_min;

    static int same_burst_count = 0;
    static int max_successful_burst = -1;
    static int same_burst_count_threshold = same_count_min;

    static Bitrates bitrates;
    if (throughput == -1) {
        throughput = (int)(STARTING_BITRATE / bitrate_throughput_ratio);
        bitrates.burst_bitrate = max_burst_bitrate;
    }

    // Make sure throughput is not negative and also that the client has received frames at all
    //     i.e., we don't want to recalculate bitrate if the video is static and server is sending
    //      zero packets
    if (stats.num_received_packets_per_second + stats.num_nacks_per_second > 0) {
        int current_throughput_heuristic =
            (int)(throughput * (1.0 * stats.num_received_packets_per_second) /
                  (1.0 * (stats.num_nacks_per_second + stats.num_received_packets_per_second)));

        if (current_throughput_heuristic == throughput) {
            // If this throughput is the same as the throughput from the last period, then
            //     increment the same throughput count
            same_throughput_count++;

            // If we have had the same throughput for the current threshold number of iterations,
            //     then set the max successful throughput to the active throughput and boost the
            //     throughput to test a higher bitrate. Reset the threshold for boosting the
            //     throughput after continuous success since we may have entered an area
            //     with higher bandwidth.
            if (same_throughput_count > same_throughput_count_threshold) {
                max_successful_throughput = throughput;
                same_throughput_count_threshold = same_count_min;
                same_throughput_count = 1;
                throughput = (int)(throughput * 1.05);
            }
        } else {
            if (throughput > max_successful_throughput && max_successful_throughput != -1) {
                // If the throughput that yielded lost packets was higher than the last
                //     continuously successful throughput, then set the throughput to the
                //     last continuously successful throughput to try it again for longer.
                //     Increase the threshold for boosting the throughput after continuous
                //     success because it is more likely that we have found our successful
                //     throughput for now.
                throughput = max_successful_throughput;
                same_throughput_count_threshold =
                    min(same_throughput_count_threshold * same_count_multiplier, same_count_max);
            } else {
                // If the throughput that yielded lost packets was lower than the last
                //     continuously successful throughput, then set the throughput to an
                //     EWMA-calculated new value and reset the threshold for boosting
                //     the throughput after continuous success.
                throughput = (int)(alpha * throughput + (1 - alpha) * current_throughput_heuristic);
                same_throughput_count_threshold = same_count_min;
            }

            same_throughput_count = 1;
        }

        bitrates.bitrate = (int)(bitrate_throughput_ratio * throughput);

        if (bitrates.bitrate > MAXIMUM_BITRATE) {
            bitrates.bitrate = MAXIMUM_BITRATE;
            throughput = (int)(MAXIMUM_BITRATE / bitrate_throughput_ratio);
        } else if (bitrates.bitrate < MINIMUM_BITRATE) {
            bitrates.bitrate = MINIMUM_BITRATE;
            throughput = (int)(MINIMUM_BITRATE / bitrate_throughput_ratio);
        }
    }

    // Make sure that frames have been recieved before trying to update the burst bitrate
    if (stats.num_rendered_frames_per_second + stats.num_skipped_frames_per_second > 0) {
        int current_burst_heuristic =
            (int)(bitrates.burst_bitrate * (1.0 * stats.num_rendered_frames_per_second) /
                  (1.0 *
                   (stats.num_skipped_frames_per_second + stats.num_rendered_frames_per_second)));

        if (current_burst_heuristic == bitrates.burst_bitrate) {
            // If this burst bitrate is the same as the burst bitrate from the last period, then
            //     increment the same burst bitrate count
            same_burst_count++;

            // If we have had the same burst bitrate for the current threshold number of iterations,
            //     then set the max successful burst bitrate to the active burst bitrate and boost
            //     the burst bitrate to test a higher burst bitrate. Reset the threshold for
            //     boosting the burst bitrate after continuous success since we may have entered an
            //     area with higher bandwidth.
            if (same_burst_count > same_burst_count_threshold) {
                max_successful_burst = bitrates.burst_bitrate;
                same_burst_count_threshold = same_count_min;
                same_burst_count = 1;
                bitrates.burst_bitrate = (int)(bitrates.burst_bitrate * 1.05);
            }
        } else {
            if (bitrates.burst_bitrate > max_successful_burst && max_successful_burst != -1) {
                // If the burst bitrate that yielded lost packets was higher than the last
                //     continuously successful burst bitrate, then set the burst bitrate to
                //     the last continuously successful burst bitrate to try it again for longer.
                //     Increase the threshold for boosting the burst bitrate after continuous
                //     success because it is more likely that we have found our successful
                //     burst bitrate for now.
                bitrates.burst_bitrate = max_successful_burst;
                same_burst_count_threshold =
                    min(same_burst_count_threshold * same_count_multiplier, same_count_max);
            } else {
                // If the burst bitrate that yielded lost packets was lower than the last
                //     continuously successful burst bitrate, then set the burst bitrate to an
                //     EWMA-calculated new value and reset the threshold for boosting
                //     the burst bitrate after continuous success.
                bitrates.burst_bitrate =
                    (int)(alpha * bitrates.burst_bitrate + (1 - alpha) * current_burst_heuristic);
                same_burst_count_threshold = same_count_min;
            }

            same_burst_count = 1;
        }

        if (bitrates.burst_bitrate > STARTING_BURST_BITRATE) {
            bitrates.burst_bitrate = STARTING_BURST_BITRATE;
        } else if (bitrates.burst_bitrate < MINIMUM_BITRATE) {
            bitrates.burst_bitrate = MINIMUM_BITRATE;
        }
    }
    return bitrates;
}

/*
============================
Public Function Implementations
============================
*/

BitrateCalculator calculate_new_bitrate = ewma_ratio_bitrate;
