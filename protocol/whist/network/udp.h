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

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>

/*
============================
Defines
============================
*/

// We should be able to fix a normal-sized WhistPacket,
// in just a single segment
#define MAX_PACKET_SEGMENT_SIZE ((int)sizeof(WhistPacket))

// Represents a WhistSegment, which will be managed by the ringbuffer
typedef struct {
    WhistPacketType whist_type;
    int id;
    unsigned short index;
    unsigned short num_indices;
    unsigned short num_fec_indices;
    unsigned short segment_size;
    bool is_a_nack;
    // Must be last, since only the first segment_size bytes will be sent
    char segment_data[MAX_PACKET_SEGMENT_SIZE];
} WhistSegment;

/*
============================
Public Functions
============================
*/

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

/*
============================
Questionable Public Functions, potentially try to remove in future organizations
============================
*/

/**
 * @brief Creates a udp listen socket, that can be used in SocketContext
 *
 * @param sock                      The socket that will be initialized
 * @param port                      The port to listen on
 * @param timeout_ms                The timeout for socket
 * @return                          0 on success, otherwise failure.
 */
// TODO: There's some weird global state-type stuff happening here
// This should be removed, something weird and confusing is happening here
int create_udp_listen_socket(SOCKET* sock, int port, int timeout_ms);

/**
 * @brief       Get the number of consecutive fully received frames of the given type available.
 *              Use this to figure out how many frames are pending to render
 * 
 * @param type  The type of frames to query for
 */
int udp_get_num_pending_frames(SocketContext* context, WhistPacketType type);

// TODO: Is needed for audio.c redundancy, but should be pulled into udp.c somehow
void udp_nack_packet(SocketContext* socket_context, WhistPacketType type, int id, int index);

/**
 * @brief                          Registers a ring buffer to reconstruct WhistPackets that can be split into smaller WhistPackets, e.g. video frames. This will hold up to num_buffer IDs for the given type.
 *                                 NOTE: This function is not thread-safe on SocketContext
 *
 * @param context                  The SocketContext that will have a ring buffer
 * @param type                     The WhistPacketType that this ring buffer will be used for
 * @param max_frame_size           The largest frame that can be saved in the ring buffer
 * @param num_buffers              The number of frames that will be stored in the ring buffer
 */
// TODO: Try to remove by making the client detect a nack buffer
void udp_register_ring_buffer(SocketContext* context, WhistPacketType type, int max_frame_size, int num_buffers);

// TODO: Remove this by pulling network-side of E2E into udp.c
timestamp_us udp_get_client_input_timestamp(SocketContext* socket_context);

#endif  // WHIST_UDP_H
