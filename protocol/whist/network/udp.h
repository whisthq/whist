#ifndef WHIST_UDP_H
#define WHIST_UDP_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file udp.h
 * @brief This file contains the network interface code for UDP protocol
 *
 *
============================
Usage
============================

To create the context:
udp_context = create_udp_network_context(...);

To send a packet:
udp_context->send_packet(...);

To read a packet:
udp_context->read_packet(...);

To free a packet:
udp_context->free_packet(...);
 */

#include <whist/core/whist.h>

/**
 * @brief Creates a udp network context and initializes a UDP connection
 *        between a server and a client
 *
 * @param context                   The socket context that will be initialized
 * @param destination               The server IP address to connect to. Passing
 *                                  NULL will wait for another client to connect
 *                                  to the socket
 * @param port                      The port to connect to. It will be a virtual
 *                                  port.
 * @param recvfrom_timeout_s        The timeout that the socketcontext will use
 *                                  after being initialized
 * @param connection_timeout_ms     The timeout that will be used when attempting
 *                                  to connect. The handshake sends a few packets
 *                                  back and forth, so the upper bound of how
 *                                  long CreateUDPSocketContext will take is
 * @param using_stun                True/false for whether or not to use the STUN server for this
 *                                  context
 *                                  some small constant times connection_timeout_ms
 * @param binary_aes_private_key    The AES private key used to encrypt the socket
 *                                  communication
 * @return                          The UDP network context on success, NULL
 *                                  on failure
 */
bool create_udp_socket_context(SocketContext* context, char* destination, int port,
                               int recvfrom_timeout_s, int connection_timeout_ms, bool using_stun,
                               char* binary_aes_private_key);

/**
 * @brief Creates a udp listen socket, that can be used in SocketContext
 *
 * @param sock                      The socket that will be initialized
 * @param port                      The port to listen on
 * @param timeout_ms                The timeout for socket
 * @return                          0 on success, otherwise failure.
 */
int create_udp_listen_socket(SOCKET* sock, int port, int timeout_ms);

/**
 * @brief                          Sets the burst bitrate for the given UDP SocketContext
 *
 * @param context                  The SocketContext that we'll be adjusting the burst bitrate for
 * @param network_settings         The new network settings
 */
void udp_update_network_settings(SocketContext* context, NetworkSettings network_settings);

/**
 * @brief                          Registers a nack buffer, so that future nacks can be handled.
 *                                 It will be able to respond to nacks from the most recent
 *                                 `buffer_size` ID's that have been send via send_packet
 *                                 NOTE: This function is not thread-safe on SocketContext
 *
 * @param context                  The SocketContext that will have a nack buffer
 * @param type                     The WhistPacketType that this nack buffer will be used for
 * @param max_frame_size           The largest frame that can be saved in the nack buffer
 * @param num_buffers              The number of buffers that will be stored in the nack buffer
 */
void udp_register_nack_buffer(SocketContext* context, WhistPacketType type, int max_frame_size,
                              int num_buffers);

/**
 * @brief                          Registers a ring buffer to reconstruct WhistPackets that can be split into smaller WhistPackets, e.g. video frames. This will hold up to num_buffer IDs for the given type.
 *                                 NOTE: This function is not thread-safe on SocketContext
 *
 * @param context                  The SocketContext that will have a ring buffer
 * @param type                     The WhistPacketType that this ring buffer will be used for
 * @param max_frame_size           The largest frame that can be saved in the ring buffer
 * @param num_buffers              The number of frames that will be stored in the ring buffer
 */
void udp_register_ring_buffer(SocketContext* context, WhistPacketType type, int max_frame_size, int num_buffers);

/**
 * @brief       Send a nack to the server indicating that the client is missing the packet with given type, ID, and index
 * @param socket_context     the context we're sending the nack over
 * @param type      type of packet
 * @param ID       ID of packet
 * @param index     index of packet
 */
int udp_nack_packet(SocketContext* socket_context, WhistPacketType type, int id, int index);

/**
 * @brief        Send a stream reset request to the server indicating that the client is stuck on ID greatest_failed_id for packets of given type
 *
 * @param socket_context     Context to send reset request over
 * @param type    type of packets that need to be reset
 * @param greatest_failed_id    Last ID the client is stuck on
 */
int udp_request_stream_reset(SocketContext* context, WhistPacketType type, int greatest_failed_id);

/**
 * @brief       Get the number of consecutive fully received frames of the given type available. Use this e.g. to predict how many ms of audio we have buffered.
 * 
 * @param type The type of frames to query for
 */
int udp_get_num_pending_frames(SocketContext* context, WhistPacketType type);

/**
 * @brief       Check if the context has received any stream reset requests for specified type
 *
 * @param socket_context    The socket context to check for reset requests
 * @param type     The type of the packet stream
 *
 * @returns a StreamResetData struct indicating if there is a pending stearm request and the last failed ID from the client
 */
StreamResetData udp_get_pending_stream_reset_request(SocketContext* socket_context, WhistPacketType type);

// TODO: document these
timestamp_us udp_get_ping_server_time(SocketContext* socket_context);
timestamp_us udp_get_ping_client_time(SocketContext* socket_context);
void udp_lock_timestamp_mutex(SocketContext* socket_context);
void udp_unlock_timestamp_mutex(SocketContext* socket_context);

#endif  // WHIST_UDP_H
