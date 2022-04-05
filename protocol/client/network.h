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
(e.g. clipboard messages) are sent over TCP; otherwise, messages are sent over UDP.
*/

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Connect to the server.
 *
 * @param server_ip                Server IP address to connect to.
 * @param using_stun               Whether we are using the STUN server
 * @param user_email               User email to provide.
 * @param handshake_sync_semaphore Sync this connect_to_server, as know as handshake,
 *                                 with other relevant concurrent initialization stuffs.
 * @returns                        Returns -1 on failure, 0 on success
 */
int connect_to_server(const char* server_ip, bool using_stun, const char* user_email,
                      WhistSemaphore handshake_sync_semaphore);

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

#endif  // CLIENT_NETWORK_H
