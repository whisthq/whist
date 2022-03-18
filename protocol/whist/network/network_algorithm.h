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

/**
 * @brief               This function will estimate the new bitrate based on whist congestion
 *                      control algo
 *
 * @param curr_group_stats Pointer to struct containing any current group of packets' departure time
 *                         and arrival time
 *
 * @param prev_group_stats Pointer to struct containing any previous group of packets' departure
 *                         time and arrival time
 *
 * @param incoming_bitrate Bitrate received over the last few hundred milliseconds. Exact duration
 *                         is specified in WCC.md
 *
 * @param packet_loss_ratio Packet loss ratio over last 250ms or so
 *
 * @param network_settings Pointer to the struct containing previous network_settings. Also the new
 *                         network settings will be updated in this struct.
 *
 * @returns             Whether network_settings struct was updated with new values or not
 */
bool whist_congestion_controller(GroupStats *curr_group_stats, GroupStats *prev_group_stats,
                                 int incoming_bitrate, double packet_loss_ratio,
                                 NetworkSettings *network_settings);

/**
 * @brief               This function will return the default network settings for a given video
 *                      resolution
 *
 * @param width         Video width
 *
 * @param height        Video height
 *
 * @param dpi           Screen dpi
 *
 * @returns             A network settings struct
 */
NetworkSettings get_default_network_settings(int width, int height, int dpi);

/**
 * @brief               This function will return the default network settings for the current
 *                      SDL window resolution.
 *                      This function will work only in client. Since server doesn't use SDL it
 *                      should use the earlier function get_default_network_settings by passing
 *                      relevant width and height.
 *
 * @returns             A network settings struct
 */
NetworkSettings get_starting_network_settings(void);

/**
 * @brief               This function will set the DPI of the screen/display.
 *                      DPI value is required to calculate bitrate limits.
 *
 * @param dpi           Screen dpi
 */
void network_algo_set_dpi(int dpi);

#endif
