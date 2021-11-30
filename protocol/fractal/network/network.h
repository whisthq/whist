#ifndef NETWORK_H
#define NETWORK_H
/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file network.h
 * @brief This file contains all code that interacts directly with sockets
 *        under-the-hood.
============================
Usage
============================

SocketContextData: This type represents a socket.
   - To use a socket, call CreateUDPContext or CreateTCPContext with the desired
     parameters
   - To send data over a socket, call SendTCPPacket or SendUDPPacket
   - To receive data over a socket, call ReadTCPPacket or ReadUDPPacket
   - If there is belief that a packet wasn't sent, you can call ReplayPacket to
     send a packet twice

FractalPacket: This type represents a packet of information
   - Unique packets of a given type will be given unique IDs. IDs are expected
     to be increasing monotonically, with a gap implying that a packet was lost
   - FractalPackets that were thought to have been sent may not arrive, and
     FractalPackets may arrive out-of-order, in the case of UDP. This will not
     be the case for TCP, however TCP sockets may lose connection if there is a
     problem.
   - A given block of data will, during transmission, be split up into packets
     with the same type and ID, but indicies ranging from 0 to num_indices - 1
   - A missing index implies that a packet was lost
   - A FractalPacket is only guaranteed to have data information from 0 to
     payload_size - 1 data[] occurs at the end of the packet, so extra bytes may
     in-fact point to invalid memory to save space and bandwidth
   - A FractalPacket may be sent twice in the case of packet recovery, but any
     two FractalPackets found that are of the same type and ID will be expected
     to have the same data (To be specific, the Client should never legally send
     two distinct packets with same ID/Type, and neither should the Server, but
if the Client and Server happen to both make a PACKET_MESSAGE packet with ID 1
     they can be different)
   - To reconstruct the original datagram from a sequence of FractalPackets,
     concatenated the data[] streams (From 0 to payload_size - 1) for each index
     from 0 to num_indices - 1

-----
Client
-----

SocketContext context;
create_tcp_socket_context(&context, "10.0.0.5", 5055, 500, 250);

char* msg = "Hello this is a message!";
send_packet(&context, PACKET_MESSAGE, msg, strlen(msg) + 1);

FractalPacket* packet = NULL;
while(packet == NULL) {
    packet = read_packet(&context);
}

LOG_INFO("Response: %s", packet->data); // Will print "Message received!"

free_packet(packet);

destroy_socket_context(&context);

-----
Server
-----

SocketContext context;
creaqte_tcp_socket_context(&context, NULL, 5055, 500, 250);

FractalPacket* packet = NULL;
while(packet == NULL) {
    packet = read_packet(&context);
}

LOG_INFO("Message: %s", packet->data); // Will print "Hello this is a message!"

free_packet(packet);

char* msg = "Message received!";
send_packet(&context, PACKET_MESSAGE, msg, strlen(msg) + 1);

destroy_socket_context(&context);

*/

/*
============================
Includes
============================
*/

// In order to use accept4 we have to allow non-standard extensions
#if !defined(_GNU_SOURCE) && defined(__linux__)
#define _GNU_SOURCE
#endif

#if defined(_WIN32)
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <fractal/network/throttle.h>

/*
============================
Defines
============================
*/

#if defined(_WIN32)
#define WHIST_ETIMEDOUT WSAETIMEDOUT
#define WHIST_EWOULDBLOCK WSAEWOULDBLOCK
#define WHIST_EAGAIN WSAEWOULDBLOCK
#define WHIST_EINPROGRESS WSAEWOULDBLOCK
#define socklen_t int
#define WHIST_IOCTL_SOCKET ioctlsocket
#define WHIST_CLOSE_SOCKET closesocket
#else
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#define WHIST_IOCTL_SOCKET ioctl
#define WHIST_CLOSE_SOCKET close
#define WHIST_ETIMEDOUT ETIMEDOUT
#define WHIST_EWOULDBLOCK EWOULDBLOCK
#define WHIST_EAGAIN EAGAIN
#define WHIST_EINPROGRESS EINPROGRESS
#endif

#define STUN_IP "0.0.0.0"
#define STUN_PORT 48800

// Note that both the Windows and Linux versions use 2 as the second argument
// to indicate shutting down both ends of the socket
#define WHIST_SHUTDOWN_SOCKET(s) shutdown(s, 2)

/*
============================
Constants
============================
*/

#define MAX_PAYLOAD_SIZE 1285
#define VIDEO_NACKBUFFER_SIZE 25
#define AUDIO_NACKBUFFER_SIZE 100

/*
============================
Custom types
============================
*/

// TODO: Unique PRIVATE_KEY for every session, so that old packets can't be
// replayed
// TODO: INC integer that must not be used twice

/**
 * @brief                          Data packet description
 */
typedef enum {
  PACKET_AUDIO = 0,
  PACKET_VIDEO = 1,
  PACKET_MESSAGE = 2,
  NUM_PACKET_TYPES = 3,
} FractalPacketType;

#include <fractal/core/fractal.h>

/**
 * @brief                          Packet of data to be sent over a
 *                                 SocketContextData
 */
typedef struct {
  char hash[16];  // Hash of the rest of the packet
  // hash[16] is a signature for everything below this line

  // Encrypted packet data
  int cipher_len;  // The length of the encrypted segment
  char iv[16];     // One-time pad for encrypted data

  // Everything below this line gets encrypted

  // Metadata
  FractalPacketType type;  // Video, Audio, or Message
  int id;                  // Unique identifier (Two packets with the same type and id, from
                           // the same IP, will be the same)
  short index;             // Handle separation of large datagrams
  short num_indices;       // The total datagram consists of data packets with
                           // indices from 0 to payload_size - 1
  int payload_size;        // size of data[] that is of interest
  bool is_a_nack;          // True if this is a replay'ed packet

  // Data
  uint8_t data[MAX_PAYLOAD_SIZE];  // data at the end of the struct, with invalid
                                   // bytes beyond payload_size / cipher_len
  uint8_t overflow[16];            // The maximum cipher_len is MAX_PAYLOAD_SIZE + 16,
                                   // as the encrypted packet might be slightly larger
                                   // than the unencrypted packet
} FractalPacket;

#define MAX_PACKET_SIZE (sizeof(FractalPacket))
#define PACKET_HEADER_SIZE (sizeof(FractalPacket) - MAX_PAYLOAD_SIZE - 16)
// Real packet size = PACKET_HEADER_SIZE + FractalPacket.payload_size (If
// Unencrypted)
//                  = PACKET_HEADER_SIZE + cipher_len (If Encrypted)

typedef struct {
  unsigned int ip;
  unsigned short private_port;
  unsigned short public_port;
} StunEntry;

typedef enum { ASK_INFO, POST_INFO } StunRequestType;

/**
 * @brief                    This enum represents a TOS header which is used
 *                           to set the type of service of a socket. Routers
 *                           may handle packets differently based on the choice
 *                           of TOS. See https://en.wikipedia.org/wiki/Differentiated_services
 *                           for DiffServ (DSCP) values, and https://www.tucny.com/Home/dscp-tos
 *                           for their corresponding TOS values.
 */
typedef enum FractalTOSValue {
  TOS_DSCP_STANDARD_FORWARDING = 0x00,  //<<< Standard Forwarding (Default)
  TOS_DSCP_EXPEDITED_FORWARDING = 0xb8  //<<< Expedited Forwarding (High Priority, for Video/Audio)
} FractalTOSValue;

typedef struct {
  StunRequestType type;
  StunEntry entry;
} StunRequest;

typedef struct {
  int timeout;
  SOCKET socket;
  struct sockaddr_in addr;
  int ack;
  WhistMutex mutex;
  char binary_aes_private_key[16];
  // Used for reading TCP packets
  int reading_packet_len;
  DynamicBuffer* encrypted_tcp_packet_buffer;
  NetworkThrottleContext* network_throttler;
  int burst_bitrate;
  double fec_packet_ratio;
  bool decrypted_packet_used;
  FractalPacket decrypted_packet;
  // Nack Buffer Data
  FractalPacket** nack_buffers[NUM_PACKET_TYPES];
  // This mutex will protect the data in nack_buffers
  WhistMutex nack_mutex[NUM_PACKET_TYPES];
  int nack_num_buffers[NUM_PACKET_TYPES];
  int nack_buffer_max_indices[NUM_PACKET_TYPES];
  int nack_buffer_max_payload_size[NUM_PACKET_TYPES];
} SocketContextData;

/**
 * @brief                       Interface describing the available functions
 *                              and socket context of a network protocol
 *
 */
typedef struct {
  // Attributes
  void* context;

  // Function table
  int (*ack)(void* context);
  FractalPacket* (*read_packet)(void* context, bool should_recv);
  void (*free_packet)(void* context, FractalPacket* packet);
  int (*send_packet)(void* context, FractalPacketType type, void* data, int len, int id);
  void (*destroy_socket_context)(void* context);
} SocketContext;

/*
============================
SocketContext Interface
============================
*/

/**
 * @brief                          Send a 0-length packet over the socket. Used
 *                                 to keep-alive over NATs, and to check on the
 *                                 validity of the socket
 *
 * @param context                  The socket context
 *
 * @returns                        Will return -1 on failure, and 0 on success
 *                                 Failure implies that the socket is
 *                                 broken or the TCP connection has ended, use
 *                                 get_last_network_error() to learn more about the
 *                                 error
 */
int ack(SocketContext* context);

/**
 * @brief                          Receive a FractalPacket from a SocketContext,
 *                                 if any such packet exists
 *
 * @param context                  The socket context
 * @param should_recv              If false, this function will only pop buffered packets,
 *                                 if any exist.
 *                                 If true, this function will call recv from the socket,
 *                                 but that might take a while in the case of TCP.
 *
 * @returns                        A pointer to the FractalPacket on success,
 *                                 NULL on failure
 */
FractalPacket* read_packet(SocketContext* context, bool should_recv);

/**
 * @brief                          Frees a FractalPacket created by read_packet
 *
 * @param context                  The socket context that created the packet
 * @param packet                   The FractalPacket to free
 */
void free_packet(SocketContext* context, FractalPacket* packet);

/**
 * @brief                          Given a FractalPacket's type, payload, payload_len, and id,
 *                                 This function will send the FractalPacket over the network.
 *                                 There is no restriction on the size of this packet,
 *                                 it will be fragmented if necessary.
 *                                 TODO: Fragment over UDP
 *
 * @param context                  The socket context
 * @param packet_type              The FractalPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param payload                  A pointer to the payload that is to be sent
 * @param payload_size             The size of the payload
 * @param packet_id                A Packet ID for the packet.
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int send_packet(SocketContext* context, FractalPacketType packet_type, void* payload,
                int payload_size, int packet_id);

/**
 * @brief                          Destroys an allocated and initialized SocketContext
 *
 * @param context                  The SocketContext to destroy
 */
void destroy_socket_context(SocketContext* context);

/*
============================
Public Functions
============================
*/

/*
 * @brief                          Initialize networking system
 *                                 Must be called before calling any other function in this file
 */
void whist_init_networking();

/**
 * @brief                          Get the most recent network error.
 *
 * @returns                        The network error that most recently occured,
 *                                 through WSAGetLastError on Windows or errno
 *                                 on Linux
 */
int get_last_network_error();

/**
 * @brief                          Get the size of a FractalPacket
 *
 * @param packet                   The packet to get the size of
 *
 * @returns                        The size of the packet, or -1 on error
 */
int get_packet_size(FractalPacket* packet);

/**
 * @brief                          This will set `socket` to have timeout
 *                                 timeout_ms.
 *
 * @param socket                   The SOCKET to be configured
 * @param timeout_ms               The maximum amount of time that all recv/send
 *                                 calls will take for that socket (0 to return
 *                                 immediately, -1 to never return). Set 0 to have
 *                                 a non-blocking socket, and -1 for an indefinitely
 *                                 blocking socket.
 */
void set_timeout(SOCKET socket, int timeout_ms);

/**
 *
 * @brief                          This will set `socket` to have the specified TOS
 *                                 value. This is used to set the type of service
 *                                 header in the packet, which is used by routers
 *                                 to determine routing/queueing/dropping behavior.
 *
 * @param socket                   The SOCKET to be configured
 * @param tos                      The TOS value to set. This should correspond to a
 *                                 DSCP value (see https://www.tucny.com/Home/dscp-tos)
 */
void set_tos(SOCKET socket, FractalTOSValue tos);

/**
 * @brief                          Perform a UDP socket() syscall and set fds to
 *                                 use flag FD_CLOEXEC if needed. This prevents
 *                                 child processes from holding onto the socket,
 *                                 preventing the socket's closure.
 *
 * @returns                        The socket file descriptor, -1 on failure
 */
SOCKET socketp_udp();

/**
 * @brief                          Perform socket syscalls and set fds to
 *                                 use flag FD_CLOEXEC
 *
 * @returns                        The socket file descriptor, -1 on failure
 */

/**
 * @brief                          Given a SocketContextData, this will perform a private key
 *                                 handshake with the other side of the connection
 *
 * @param context                  The SocketContextData that the handshake will happen over
 */
bool handshake_private_key(SocketContextData* context);

// Included at the bottom due to circular #include <network.h> reference
#include <fractal/network/tcp.h>
#include <fractal/network/udp.h>

#endif  // NETWORK_H
