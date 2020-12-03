#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.h
 * @brief This file contains server-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

*/

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
 * @param id                       An ID for the UDP data.
 * @param burst_bitrate            The maximum bitrate that packets will be sent
 *                                 over. -1 will imply sending as fast as
 *                                 possible
 * @param packet_buffer            An array of RTPPacket's, each sub-packet of
 *                                 the UDPPacket will be stored in
 *                                 packet_buffer[i]
 * @param packet_len_buffer        An array of int's, defining the length of
 *                                 each sub-packet located in packet_buffer[i]
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int broadcast_udp_packet(FractalPacketType type, void *data, int len, int id, int burst_bitrate,
                         FractalPacket *packet_buffer, int *packet_len_buffer);

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
int broadcast_tcp_packet(FractalPacketType type, void *data, int len);

/**
 * @brief                          Tries to read in next available TCP message
 *                                 from a given, active client
 *
 * details                         If no TCP message is available, *fcmsg is
 *                                 set to NULL and *fcmsg_size is set to 0.
 *                                 Otherwise, *fcmsg is populated with a pointer
 *                                 to the next available TCP message and
 *                                 *fcmsg_size is set to the size of that
 *                                 message. The message need not be freed.
 *                                 Failure here
 *
 * @param client_id                Client ID of an active client
 * @param fcmsg                    Pointer to field which is to be populated
 *                                 with pointer to next available message
 * @param fcmsg_size               Pointer to field which is to be populated
 *                                 with size of the next available message.
 *
 * @returns                        Returns -1 on error, 0 otherwise. Not
 *                                 finding an available message is not an error.
 */
int try_get_next_message_tcp(int client_id, FractalClientMessage **fcmsg, size_t *fcmsg_size);

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
 * @param client_id                Client ID of an active client
 * @param fcmsg                    Pointer to field which is to be populated
 *                                 with pointer to next available message
 * @param fcmsg_size               Pointer to field which is to be populated
 *                                 with size of the next available message.
 *
 * @returns                        Returns -1 on error, 0 otherwise. Not
 *                                 finding an available message is not an error.
 */
int try_get_next_message_udp(int client_id, FractalClientMessage *fcmsg, size_t *fcmsg_size);

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
 * @param id                       Client ID of an active client
 * @param binary_aes_private_key          Key used to encrypt and decrypt communication
 *                                 with the client.
 *
 * @returns                        Returns -1 if either UDP or TCP connection
 *                                 fails or another error occurs, 0 on success.
 */
int connect_client(int id, char *binary_aes_private_key);

/**
 * @brief                          Closes client's UDP and TCP sockets.
 *
 * @param id                       Client ID of an active client
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int disconnect_client(int id);

/**
 * @brief                          Closes UDP and TCP sockets for all active
 *                                 clients.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int disconnect_clients(void);

#endif  // SERVER_NETWORK_H
