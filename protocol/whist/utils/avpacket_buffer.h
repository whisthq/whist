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
 * @brief                       Store num_packets AVPackets, found in packets, into
 *                              a pre-allocated buffer.
 *
 * @param buffer                Buffer to store packet data in
 *
 * @param buffer_size           The size of the buffer
 *
 * @param packets               Array of AVPackets to store
 *
 * @param num_packets           Size of the packets array
 *
 * @return                      The size of the resultant buffer. (Bytes written to)
 *                              Or -1, if the buffer was not big enough
 */
int write_avpackets_to_buffer(void* buffer, int buffer_size, AVPacket* packets, int num_packets);

/**
 * @brief                       Read out the packets stored in buffer into the
 *                              AVPacket array packets.
 *
 * @param packets               AVPacket array to store encoded packets.
 *                              Each element will be set to NULL or a valid AVPacket pointer.
 *                              It is the caller's responsibility to `av_packet_free` them.
 *
 * @param max_num_packets       The max size of the AVPacket array
 *
 * @param buffer                Buffer containing encoded packets.
 *
 * @param buffer_size           Size of the buffer.
 *                              Must agree with return value of `write_avpackets_to_buffer`.
 *
 * @returns                     The number of extracted packets
 */
int extract_avpackets_from_buffer(AVPacket** packets, int max_num_packets, void* buffer,
                                  int buffer_size);

#endif  // DECODE_H
