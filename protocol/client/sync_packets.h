#ifndef CLIENT_SYNC_PACKETS_H
#define CLIENT_SYNC_PACKETS_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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
Includes
============================
*/

#include "renderer.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the packet synchronizer
 *                                 threads for UDP and TCP
 *
 * @param whist_renderer           The whist renderer to pass packets into
 */
void init_packet_synchronizers(WhistRenderer* whist_renderer);

/**
 * @brief                          Destroy the packet synchronizer
 *                                 threads for UDP and TCP
 */
void destroy_packet_synchronizers(void);

#endif
