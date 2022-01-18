#ifndef CLIENT_BITRATE_H
#define CLIENT_BITRATE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network_algorithm.h
 * @brief     This file contains the client's network algorithm code.
 *            It takes in any network statistics,
 *            and returns a network settings decision based on those statistics.
============================
Usage
============================
The client should periodically call calculate_new_bitrate with any new network statistics, such as
throughput or number of nacks. This will return a new network settings as needed
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>

/*
============================
Defines
============================
*/

// Max/Min/Starting Bitrates/Burst Bitrates

#define MAXIMUM_BITRATE 30000000
#define MINIMUM_BITRATE 2000000
#define STARTING_BITRATE_RAW 15400000
#define STARTING_BITRATE (min(max(STARTING_BITRATE_RAW, MINIMUM_BITRATE), MAXIMUM_BITRATE))

#define MAXIMUM_BURST_BITRATE 200000000
#define MINIMUM_BURST_BITRATE 4000000
#define STARTING_BURST_BITRATE_RAW 100000000
#define STARTING_BURST_BITRATE \
    (min(max(STARTING_BURST_BITRATE_RAW, MINIMUM_BURST_BITRATE), MAXIMUM_BURST_BITRATE))

// The subset of the bitrate that will be audio
#define AUDIO_BITRATE 128000

// The FEC Ratio to use on video/audio packets respectively
// (Only used for testing phase of FEC)
// This refers to the percentage of packets that will be FEC packets
#define AUDIO_FEC_RATIO 0.0
#define VIDEO_FEC_RATIO 0.0

typedef struct {
    int num_nacks_per_second;
    int num_received_packets_per_second;
    int num_skipped_frames_per_second;
    int num_rendered_frames_per_second;
    int throughput_per_second;
    double average_delay_gradient;
    double delay_gradient_variance;
    double average_client_side_delay;
    double average_one_way_trip_latency;
    int num_delay_measurements;
    // True so that a NetworkStatistics struct can be marked as valid/invalid
    bool statistics_gathered;
} NetworkStatistics;

/*
============================
Public Functions
============================
*/

/**
 * @brief               This function will calculate what network settings
 *                      are desired, in response to any network statistics given
 *
 * @param stats         A struct containing any information we might need
 *                      when deciding what bitrate we want
 *
 * @returns             A network settings struct
 */
NetworkSettings get_desired_network_settings(NetworkStatistics stats);

#endif
