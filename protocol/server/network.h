#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file network.h
 * @brief This file contains server-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

*/

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
 * @brief                          Broadcasts acks to all active clients
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int broadcast_ack(void);

/**
 * @brief                          Sends a FractalPacket and accompanying
 *                                 FractalPacketType to all active clients,
 *                                 over UDP.
 *
 * @param type                     The FractalPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The number of bytes to send
 * @param packet_id                       An ID for the UDP data.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int broadcast_udp_packet_from_payload(FractalPacketType type, void *data, int len, int packet_id);

/**
 * @brief                          This will send a FractalPacket over UDP to
 *                                 all active clients. This function does not
 *                                 create the packet from raw data, but assumes
 *                                 that the packet has been prepared by the
 *                                 caller (e.g. fragmented into
 *                                 appropriately-sized chunks by a fragmenter).
 *                                 This function assumes and checks that the
 *                                 packet is small enough to send without
 *                                 further breaking into smaller packets.
 *
 * @param packet                   A pointer to the packet to be sent
 * @param packet_size              The size of the packet to be sent
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int broadcast_udp_packet(FractalPacket *packet, size_t packet_size);

/**
 * @brief                          Sends a FractalPacket and accompanying
 *                                 FractalPacketType to all active clients,
 *                                 over TCP.
 *
 * @param type                     The FractalPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The nubmer of bytes to send
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int broadcast_tcp_packet_from_payload(FractalPacketType type, void *data, int len);

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
 * @param p_tcp_packet             Where to write the tcp_packet
 *
 * @returns                        Returns -1 on error, 0 otherwise. Not
 *                                 finding an available message is not an error.
 */
int try_get_next_message_tcp(FractalPacket **p_tcp_packet);

/**
 * @brief                          Tries to read in next available UDP message
 *                                 from a given, active client
 *
 * details                         If no UDP message is available, *fcmsg is
 *                                 set to NULL and *fcmsg_size is set to 0.
 *                                 Otherwise, *fcmsg is populated with a pointer
 *                                 to the next available UDP message and
 *                                 *fcmsg_size is set to the size of that
 *                                 message. The message need not be freed.
 *                                 Failure here
 *
 * @param fcmsg                    Pointer to field which is to be populated
 *                                 with pointer to next available message
 * @param fcmsg_size               Pointer to field which is to be populated
 *                                 with size of the next available message.
 *
 * @returns                        Returns -1 on error, 0 otherwise. Not
 *                                 finding an available message is not an error.
 */
int try_get_next_message_udp(FractalClientMessage *fcmsg, size_t *fcmsg_size);

/**
 * @brief                          Establishes UDP and TCP connection to client.
 *
 * details                         If no UDP message is available, *fcmsg is
 *                                 set to NULL and *fcmsg_size is set to 0.
 *                                 Otherwise, *fcmsg is populated with a pointer
 *                                 to the next available UDP message and
 *                                 *fcmsg_size is set to the size of that
 *                                 message. The message need not be freed.
 *                                 Failure here
 *
 * @param using_stun               True if we are running a stun server
 * @param binary_aes_private_key   Key used to encrypt and decrypt communication
 *                                 with the client.
 *
 * @returns                        Returns -1 if either UDP or TCP connection
 *                                 fails or another error occurs, 0 on success.
 */
int connect_client(bool using_stun, char *binary_aes_private_key);

/**
 * @brief                          Closes client's UDP and TCP sockets.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int disconnect_client();

/**
 * @brief                          Decides whether the server is using stun.
 *
 * @returns                        Returns true if server is using stun,
 *                                 false otherwise.
 */
bool get_using_stun();

/**
 * @brief                          Should be run in its own thread. Loops
 *                                 and manages connecting to/maintaining client
 *                                 connection and recovery.
 *
 * @returns                        Returns 0 on completion.
 */
int multithreaded_manage_client(void *opaque);

#endif  // SERVER_NETWORK_H
