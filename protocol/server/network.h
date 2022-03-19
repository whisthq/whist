#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network.h
 * @brief This file contains server-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

*/

#include "client.h"

/*
============================
Constants
============================
*/

#define CLIENT_PING_TIMEOUT_SEC 3.0

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Sends a WhistPacket and accompanying
 *                                 WhistPacketType to all active clients,
 *                                 over TCP.
 *
 * @param client				   The target client
 * @param type                     The WhistPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The nubmer of bytes to send
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int broadcast_tcp_packet(Client *client, WhistPacketType type, void *data, int len);

/**
 * @brief                          Tries to read in next available TCP message
 *                                 from a given, active client
 *
 * details                         If no TCP message is available, *tcp_packet
 *                                 is set to NULL.
 *                                 Otherwise, *tcp_packet is populated with a
 *                                 pointer to the next available TCP message.
 *                                 Remember to free with free_tcp_packet
 *
 * @param client				   The target client
 * @param p_tcp_packet             Where to write the tcp_packet
 *
 * @returns                        Returns -1 on error, 0 otherwise. Not
 *                                 finding an available message is not an error.
 */
int try_get_next_message_tcp(Client *client, WhistPacket **p_tcp_packet);

/**
 * @brief                          Tries to read in next available UDP message
 *                                 from a given, active client
 *
 * details                         If no UDP message is available, *wsmsg is
 *                                 set to NULL and *wsmsg_size is set to 0.
 *                                 Otherwise, *wsmsg is populated with a pointer
 *                                 to the next available UDP message and
 *                                 *wsmsg_size is set to the size of that
 *                                 message. The message need not be freed.
 *                                 Failure here
 *
 * @param client				   The target client
 * @param wsmsg                    Pointer to field which is to be populated
 *                                 with pointer to next available message
 * @param wsmsg_size               Pointer to field which is to be populated
 *                                 with size of the next available message.
 *
 * @returns                        Returns -1 on error, 0 otherwise. Not
 *                                 finding an available message is not an error.
 */
int try_get_next_message_udp(Client *client, WhistClientMessage *wsmsg, size_t *wsmsg_size);

/**
 * @brief                          Decides whether the server is using stun.
 *
 * @returns                        Returns true if server is using stun,
 *                                 false otherwise.
 */
bool get_using_stun(void);

// TODO: Move to client.h
/**
 * @brief                          Attempts to connect the client
 *
 * @param state                    The server state
 *
 * @returns                        Returns true if successfully connected
 */
bool connect_client(void *state);

#endif  // SERVER_NETWORK_H
