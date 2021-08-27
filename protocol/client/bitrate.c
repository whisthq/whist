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
    static const int same_count_min = 20;

    static int throughput = -1;
    static int same_throughput_count = 0;
    static int prev_throughput = -1;
    static int same_burst_count = 0;
    static int prev_burst_bitrate = -1;
    static Bitrates bitrates;
    if (throughput == -1) {
        throughput = (int)(STARTING_BITRATE / bitrate_throughput_ratio);
        bitrates.burst_bitrate = max_burst_bitrate;
    }

    // Make sure throughput is not negative and also that the client has received frames at all
    //     i.e., we don't want to recalculate bitrate if the video is static and server is sending
    //      zero packets
    if (stats.num_received_packets_per_second + stats.num_nacks_per_second > 0) {
        throughput =
            (int)(alpha * throughput +
                  (1 - alpha) * throughput * (1.0 * stats.num_received_packets_per_second) /
                      (1.0 * (stats.num_nacks_per_second + stats.num_received_packets_per_second)));
        LOG_INFO("MBPS NEW THROUGHPUT: %d", throughput);
        LOG_INFO("MBPS NUM NACKED: %d RATIO: %f", stats.num_nacks_per_second,
                 (1.0 * stats.num_received_packets_per_second) /
                     (1.0 * (stats.num_nacks_per_second + stats.num_received_packets_per_second)));

        // If no NACKS after `same_count_min` iterations, gradually increase bitrate with each
        // iteration
        if (throughput == prev_throughput) {
            same_throughput_count++;
        } else {
            same_throughput_count = 1;
        }
        if (same_throughput_count > same_count_min) {
            throughput *= 1.05;
        }

        bitrates.bitrate = (int)(bitrate_throughput_ratio * throughput);

        if (bitrates.bitrate > MAXIMUM_BITRATE) {
            bitrates.bitrate = MAXIMUM_BITRATE;
            throughput = (int)(MAXIMUM_BITRATE / bitrate_throughput_ratio);
        } else if (bitrates.bitrate < MINIMUM_BITRATE) {
            bitrates.bitrate = MINIMUM_BITRATE;
            throughput = (int)(MINIMUM_BITRATE / bitrate_throughput_ratio);
        }

        prev_throughput = throughput;
    }

    // Make sure that frames have been recieved before trying to update the burst bitrate
    if (stats.num_rendered_frames_per_second + stats.num_skipped_frames_per_second > 0) {
        bitrates.burst_bitrate = (int)(alpha * bitrates.burst_bitrate +
                                       (1 - alpha) * bitrates.burst_bitrate *
                                           (1.0 * stats.num_rendered_frames_per_second) /
                                           (1.0 * (stats.num_skipped_frames_per_second +
                                                   stats.num_rendered_frames_per_second)));
        LOG_INFO("MBPS NEW BURST BITRATE: %d", bitrates.burst_bitrate);
        LOG_INFO("MBPS NUM Skipped: %d", stats.num_skipped_frames_per_second);

        // If no skipped frames after `same_count_min` iterations, gradually increase bitrate with
        // each iteration
        if (bitrates.burst_bitrate == prev_burst_bitrate) {
            same_burst_count++;
        } else {
            same_burst_count = 1;
        }
        if (same_burst_count > same_count_min) {
            bitrates.burst_bitrate *= 1.05;
        }

        if (bitrates.burst_bitrate > STARTING_BURST_BITRATE) {
            bitrates.burst_bitrate = STARTING_BURST_BITRATE;
        } else if (bitrates.burst_bitrate < MINIMUM_BITRATE) {
            bitrates.burst_bitrate = MINIMUM_BITRATE;
        }

        prev_burst_bitrate == bitrates.burst_bitrate;
    }
    return bitrates;
}

/*
============================
Public Function Implementations
============================
*/

BitrateCalculator calculate_new_bitrate = ewma_ratio_bitrate;
