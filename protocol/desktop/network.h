#ifndef DESKTOP_NETWORK_H
#define DESKTOP_NETWORK_H

#include "../fractal/core/fractal.h"

/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

TODO
*/

// must be called before connectToServer()
int discover_ports(bool* using_stun);

// must be called after discoverPorts()
int connect_to_server(bool using_stun);

int close_connections(void);

/**
 * @brief                          Sends quit messages to the server
 *
 * @details                        Sends num_messages many quit messages to the
 *                                 server. Sleeps for 50 milliseconds before
 *                                 each message.
 *
 * @param num_messages             Number of quit messages to send
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int send_server_quit_messages(int num_messages);

/**
 * @brief                          Send a FractalMessage from client to server
 *
 * @param fmsg                     FractalMessage struct to send as packet
 *
 * @returns                        0 if succeeded, else -1
 */
int send_fmsg(FractalClientMessage* fmsg);

#endif  // DESKTOP_NETWORK_H
