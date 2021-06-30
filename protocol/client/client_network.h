#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <fractal/core/fractal.h>

/**
 * Copyright Fractal Computers, Inc. 2020
 * @file client_network.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

Use these functions for any client-specific networking needs.
*/

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Send a discovery packet to the server to determine which TCP
 *                                 and UDP packets are assigned to the client. Must be called
 *                                 before `connect_to_server()`.
 *
 * @param using_stun               Whether we are using the STUN server
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int discover_ports(bool* using_stun);

/**
 * @brief                          Connect to the server. Must be called after
 *                                 `discover_ports()`.
 *
 * @param using_stun               Whether we are using the STUN server
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
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

#endif  // CLIENT_NETWORK_H
