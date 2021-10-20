#ifndef UDP_H
#define UDP_H

#include "network.h"

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
 *                                  long CreateUDPNetworkContext will take is
 *                                  some small constant times connection_timeout_ms
 * @param binary_aes_private_key    The AES private key used to encrypt the socket
 *                                  communication
 * @return                          The UDP network context on success, NULL
 *                                  on failure
 */
NetworkContext* create_udp_network_context(SocketContext* context, char* destination, int port,
                                           int recvfrom_timeout_s, int connection_timeout_ms,
                                           char* binary_aes_private_key);

#endif  // UDP_H