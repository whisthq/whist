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
 *                                 over UDP.
 * @param client				   The destination client
 * @param type                     The WhistPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The number of bytes to send
 * @param packet_id                       An ID for the UDP data.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int broadcast_udp_packet(Client *client, WhistPacketType type, void *data, int len, int packet_id);

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
 * @brief                          Establishes UDP and TCP connection to client.
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
 * @param using_stun               True if we are running a stun server
 * @param binary_aes_private_key   Key used to encrypt and decrypt communication
 *                                 with the client.
 *
 * @returns                        Returns -1 if either UDP or TCP connection
 *                                 fails or another error occurs, 0 on success.
 */
int connect_client(Client *client, bool using_stun, char *binary_aes_private_key);

/**
 * @brief                          Closes client's UDP and TCP sockets.
 *
 * @param client				   The target client
 * @returns                        Returns -1 on failure, 0 on success.
 */
int disconnect_client(Client *client);

/**
 * @brief                          Decides whether the server is using stun.
 *
 * @returns                        Returns true if server is using stun,
 *                                 false otherwise.
 */
bool get_using_stun(void);

/**
 * @brief                          Should be run in its own thread. Loops
 *                                 and manages connecting to/maintaining client
 *                                 connection and recovery.
 *
 * @returns                        Returns 0 on completion.
 */
int multithreaded_manage_client(void *opaque);

#endif  // SERVER_NETWORK_H
