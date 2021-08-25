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
    static int same_throughput_count = 0;
    static int prev_throughput = -1;
    static Bitrates bitrates;
    if (throughput == -1) {
        throughput = (int)(STARTING_BITRATE / bitrate_throughput_ratio);
    }

    // Make sure throughput is not negative and also that the client has received frames at all
    //     i.e., we don't want to recalculate bitrate if the video is static and server is sending
    //      zero frames
    if (stats.throughput_per_second >= 0 && stats.throughput_per_second + stats.num_nacks_per_second > 0) {
        throughput = (int)(alpha * throughput +
            (1 - alpha) * throughput * (1.0 * stats.throughput_per_second) /
                (1.0 * (stats.num_nacks_per_second + stats.throughput_per_second)));
        LOG_INFO("MBPS NEW THROUGHPUT: %d", throughput);
        LOG_INFO("MBPS NUM NACKED: %d RATIO: %f", stats.num_nacks_per_second, (1.0 * stats.throughput_per_second) /
                (1.0 * (stats.num_nacks_per_second + stats.throughput_per_second)));

        // If no NACKS after 5 iterations, gradually increase bitrate with each iteration
        if (throughput == prev_throughput) {
            same_throughput_count++;
        } else {
            same_throughput_count = 1;
        }

        if (same_throughput_count > 5) {
            throughput *= 1.05;
        }

        bitrates.bitrate = (int)(bitrate_throughput_ratio * throughput);

        prev_throughput = throughput;
        // TODO: increase bitrate if nothing has been nacked in some time
    }
    // TODO: update burst_bitrate based on skipped renders
    // if (stats.throughput_per_second >= 0 && stats.throughput_per_second + stats.)
    bitrates.burst_bitrate = max_burst_bitrate;
    return bitrates;
}

/*
============================
Public Function Implementations
============================
*/

BitrateCalculator calculate_new_bitrate = ewma_bitrate;
