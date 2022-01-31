#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <whist/core/whist.h>

/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network.h
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================
discover_ports, connect_to_server, close_connections, and send_server_quit_messages are used to
start and end connections to the Whist server. To connect, call discover_ports, then
connect_to_server. To disconnect, send_server_quit_messages and then close_connections.

To communicate with the server, use send_wcmsg to send Whist messages to the server. Large wcmsg's
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

/**
 * @brief                          Send a TCP socket reset message to the server,
 *                                 regardless of the initiator of the lost connection.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int send_tcp_reconnect_message(void);

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
 * @brief                          Send a WhistMessage from client to server
 *
 * @param wcmsg                     WhistMessage struct to send as packet
 *
 * @returns                        0 if succeeded, else -1
 */
int send_wcmsg(WhistClientMessage* wcmsg);

/**
 * @brief                           Send a ping with ID ping_id to the server
 */
void send_ping(int ping_id);

/**
 * @brief                           Send a TCP ping with to the server with the given `ping_id`.
 *
 * @param ping_id                   Ping ID to send to the server
 */
void send_tcp_ping(int ping_id);

/**
 * @brief                           Handle a TCP pong (ping acknowledgement) with ID pong_id
 *
 * @param pong_id                   ID of the received TCP pong
 */
void receive_tcp_pong(int pong_id);

#endif  // CLIENT_NETWORK_H
