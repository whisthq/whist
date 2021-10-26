#ifndef TCP_STUN_H
#define TCP_STUN_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file tcp_stun.h
 * @brief This file contains the network interface code for TCP STUN protocol
 *
 *
============================
Usage
============================

To create the context:
tcp_stun_context = create_tcp_stun_network_context(...);

To send a packet from payload:
tcp_stun_context->send_packet_from_payload(...);

To read a packet:
tcp_stun_context->read_packet(...);

To free a packet:
tcp_stun_context->free_packet(...);
 */

#include "network.h"

/**
 * @brief Creates a tcp stun network context and initializes a TCP STUN connection
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
 *                                  long CreateUDPNetworkContext will take is
 *                                  some small constant times connection_timeout_ms
 * @param binary_aes_private_key    The AES private key used to encrypt the socket
 *                                  communication
 * @return                          The TCP STUN network context on success, NULL
 *                                  on failure
 */
NetworkContext* create_tcp_stun_network_context(SocketContext* context, char* destination, int port,
                                                int recvfrom_timeout_s, int connection_timeout_ms,
                                                char* binary_aes_private_key);

#endif  // TCP_STUN_H
