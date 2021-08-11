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
#include "bitrate.h"

volatile int max_bitrate = STARTING_BITRATE;
volatile int max_burst_bitrate = STARTING_BURST_BITRATE;
#define BAD_BITRATE 10400000
#define BAD_BURST_BITRATE 31800000

/*
============================
Private Functions
============================
*/
void fallback_bitrate(int num_nacks_per_second);

/*
============================
Private Function Implementations
============================
*/
void fallback_bitrate(int num_nacks_per_second) {
    /*
        Switches between two sets of bitrate/burst bitrate: the default of 16mbps/100mbps and a
       fallback of 10mbps/30mbps. We fall back if we've nacked a lot in the last second.

        Arguments:
            num_nacks_per_second (int): the average number of nacks per second since the last time
       this function was called
    */
    if (num_nacks_per_second > 6 && max_bitrate != BAD_BITRATE) {
        max_bitrate = BAD_BITRATE;
        max_burst_bitrate = BAD_BURST_BITRATE;
    }
}

/*
============================
Public Function Implementations
============================
*/
bool calculate_new_bitrate(int num_nacks_per_second) {
    /*
        Update max_bitrate and max_burst_bitrate (if necessary) using client data. Returns whether
       or not the client should request bitrate changes from the server.

        Arguments:
            num_nacks_per_second (int): number of nacks per second since this function was last
       called

        Returns:
            (bool): whether or not the client should request a bitrate change from the server
    */
    int old_bitrate = max_bitrate;
    fallback_bitrate(num_nacks_per_second);
    bool should_update = (old_bitrate != max_bitrate) &&
                         (max_bitrate == STARTING_BITRATE || max_bitrate == BAD_BITRATE);
    return should_update;
}
