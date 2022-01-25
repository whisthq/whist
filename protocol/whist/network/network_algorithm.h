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
    // goog_cc stats
    bool goog_cc_ready;
    double delay_gradient;
    double client_side_delay;
    int one_way_trip_time;
    // True so that a NetworkStatistics struct can be marked as valid/invalid
    bool statistics_gathered;
} NetworkStatistics;

extern const NetworkSettings default_network_settings;

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

/**
 * @brief               This function simulates the google congestion control
 *                      loss controller given network stats and delay estimated bitrate
 *
 * @param stats         A struct containing any information we might need
 *                      when deciding what bitrate we want
 *
 * @param receiver_estimated_maximum_bitrate    Output of the goog_cc delay controller
 *
 * @returns             Integer representing loss controller set bitrate
 */
int goog_cc_loss_controller(NetworkStatistics stats, int receiver_estimated_maximum_bitrate);

/**
 * @brief               This function simulates the google congestion control
 *                      delay controller given network stats
 *
 * @param stats         A struct containing network information including
 *                      delay gradient, one-way trip time, and packets per second
 *
 * @returns             Integer representing delay controller estimated maximum bitrate
 */
int goog_cc_delay_controller(NetworkStatistics stats);

#endif
