#ifndef ENCODE_H
#define ENCODE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file encode.h
 * @brief This file contains helper functions for audio/video encoding.
============================
Usage
============================

Use write_packets_to_buffer to store an array of AVPackets into a buffer that includes metadata
about number of packets and packet size. To read the packets from the buffer, use
extract_packets_from_buffer.
*/

/*
============================
Includes
============================
*/
#include <fractal/core/fractal.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief													Store num_packets AVPackets, found in packets, into a
 * pre-allocated buffer.
 *
 * @param num_packets							Number of packets to store from packets
 *
 * @param packets									Array of packets to store
 *
 * @param buf											Buffer to store packets in. Format: (number of packets)(size of
 * each packet)(data of each packet)
 */
void write_packets_to_buffer(int num_packets, AVPacket* packets, int* buf);

#endif  // ENCODE_H
