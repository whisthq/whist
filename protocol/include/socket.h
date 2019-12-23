/*
 * This file contains the headers of helper functions regarding sockets on Linux
 * Ubuntu, specifically UDP sockets, that are used by the hole punching server
 * to communicate with the peers.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modification: 12/22/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (_WIN32)
  #include <winsock2.h>
  #include <windows.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_lib")
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
#endif

// @brief ensures reliable UDP sending over a socket by listening for an ack
// @details requires complementary reliable_udp_recvfrom on the receiving end
int reliable_udp_sendto(int socket_fd, unsigned char *message, int message_len, struct sockaddr_in dest_addr, socklen_t addr_size);

// @brief ensures reliable UDP receiving over a socket by listening for an ack
// @details requires complementary reliable_udp_sendto on the sending end
int reliable_udp_recvfrom(int socket_fd, char *msg_buff, int msg_bufflen, struct sockaddr_in dest_addr, socklen_t addr_size, int timeout);
