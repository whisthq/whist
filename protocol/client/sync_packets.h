#ifndef CLIENT_SYNC_PACKETS_H
#define CLIENT_SYNC_PACKETS_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file sync_packets.h
 * @brief This file contains code for high-level communication with the server
============================
Usage
============================
Use `init_packet_synchronizers()` to spin up threads for handling UDP messages (video, audio, etc.)
and TCP messages (clipboard, files, etc.). Use `destroy_packet_synchronizers()` to shut them down
and wait for them to finish.
*/

/*
============================
Public Functions
============================
*/

/**
 * @brief        Initialize the packet synchronizer threads for UDP and TCP.
 */
void init_packet_synchronizers();

/**
 * @brief        Destroy and wait on the packet synchronizer threads for UDP and TCP.
 */
void destroy_packet_synchronizers();

#endif
