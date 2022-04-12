#ifndef _AVPACKET_BUFFER_H_
#define _AVPACKET_BUFFER_H_

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file avpacket_buffer.h
 * @brief This file contains helper functions for decoding video and audio.
============================
Usage
============================

Use extract_avpackets_from_buffer to read packets from a buffer which has been filled using
write_avpackets_to_buffer.
*/

/*
============================
Includes
============================
*/
#include <whist/core/whist.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief                       Read out the packets stored in buffer into the
 *                              AVPacket array packets.
 *
 * @param buffer                Buffer containing encoded packets. Format: (number of packets)(size
 *                              of each packet)(data of each packet)
 *
 * @param buffer_size           Size of the buffer. Must agree with the metadata given
 *                              in buffer.
 *
 * @param packets               AVPacket array to store encoded packets
 *
 * @returns                     0 on success, -1 on failure
 */
int extract_avpackets_from_buffer(uint8_t* buffer, size_t buffer_size, AVPacket** packets);

/**
 * @brief                       Store num_packets AVPackets, found in packets, into
 *                              a pre-allocated buffer.
 *
 * @param num_packets           Number of packets to store from packets
 *
 * @param packets               Array of packets to store
 *
 * @param buffer                Buffer to store packets in. Format: (number of packets)(size
 * 								of each packet)(data of each packet)
 */
void write_avpackets_to_buffer(int num_packets, AVPacket** packets, uint8_t* buffer);

#endif  // DECODE_H
