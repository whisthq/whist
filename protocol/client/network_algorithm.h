#ifndef CLIENT_BITRATE_H
#define CLIENT_BITRATE_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file bitrate.h
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
Custom Types
============================
*/

typedef struct {
    int num_nacks_per_second;
    int num_received_packets_per_second;
    int num_skipped_frames_per_second;
    int num_rendered_frames_per_second;
    int throughput_per_second;
} NetworkStatistics;

typedef struct {
    int fps;
    int bitrate;
    int burst_bitrate;
    double fec_packet_ratio;
    CodecType desired_codec;
} NetworkSettings;

/*
============================
Public Functions
============================
*/

/**
 * @brief               Update max_bitrate and max_burst_bitrate with the latest client data.
 *                      This function does not trigger any client bitrate updates.
 *
 * @param stats         A struct containing any information we might need
 *                      when deciding what bitrate we want
 *
 * @returns             A network settings struct
 */
NetworkSettings calculate_new_bitrate(NetworkStatistics stats);

#endif
