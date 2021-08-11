#ifndef CLIENT_BITRATE_H
#define CLIENT_BITRATE_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file bitrate.h
 * @brief This file contains the client's adaptive bitrate code. Any algorithms we are using for
predicting bitrate should be stored here.
============================
Usage
============================
The client should periodically call calculate_new_bitrate with any data rrequested, such as
throughput or nack data. This will update max_bitrate and max_burst_bitrate if needed.
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
 * @brief       Update max_bitrate and max_burst_bitrate with the latest client data. Return whether
 * or not those bitrates have actually changed. This function does not trigger an update bitrate
 * message to the server.
 *
 * @param num_nacks         The number of nacks that occurred in the last secod
 *
 * @return      Whether or not we changed max_bitrate and max_burst_bitrate
 */
bool calculate_new_bitrate(int num_nacks);
#endif
