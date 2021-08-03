#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <fractal/core/fractal.h>

/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================
discover_ports, connect_to_server, close_connections, and send_server_quit_messages are used to
start and end connections to the Fractal server. To connect, call discover_ports, then
connect_to_server. To disconnect, send_server_quit_messages and then close_connections.

To communicate with the server, use send_fmsg to send Fractal messages to the server. Large fmsg's
(e.g. clipboard messages) are sent over TCP; otherwise, messages are sent over UDP. Use update_ping
to ping the server at regular intervals, and receive_pong to receive pongs (ping acknowledgements)
from the server.
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

/**
 * @brief                           Handle pinging the server if enough time has passed
 */
void update_ping();

/**
 * @brief                           Handle a pong (ping acknowledgement) with ID pong_id
 *
 * @param pong_id                   ID of the received pong
 */
void receive_pong(int pong_id);

#endif  // CLIENT_NETWORK_H
