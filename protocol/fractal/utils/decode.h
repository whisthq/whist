#ifndef DECODE_H
#define DECODE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file decode.h
 * @brief This file contains helper functions for decoding video and audio.
============================
Usage
============================

Use extract_packets_from_buffer to read packets from a buffer which has been filled using
write_packets_to_buffer.
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
 * @brief 											Read out the packets stored in buffer into the AVPacket
 * array packets.
 *
 * @param buffer								Buffer containing encoded packets. Format: (number of packets)(size of
 * each packet)(data of each packet)
 *
 * @param buffer_size						Size of the buffer. Must agree with the metadata given
 * in buffer.
 *
 * @param packets								AVPacket array to store encoded packets
 *
 * @returns											0 on success, -1 on failure
 */
int extract_packets_from_buffer(void* buffer, int buffer_size, AVPacket* packets);

#endif  // DECODE_H
