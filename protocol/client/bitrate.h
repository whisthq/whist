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
Custom Types
============================
*/

typedef struct BitrateStatistics {
    int num_nacks_per_second;
    int throughput_per_second;
} BitrateStatistics;

typedef struct Bitrates {
    int bitrate;
    int burst_bitrate;
} Bitrates;

typedef Bitrates (*BitrateCalculator)(BitrateStatistics);

/*
============================
Public Functions
============================
*/

/**
 * @brief       Update max_bitrate and max_burst_bitrate with the latest client data. This function does not trigger any client bitrate updates.
 *
 * @param stats         A struct containing any information we might need to update bitrate.
 */
BitrateCalculator calculate_new_bitrate;
#endif
