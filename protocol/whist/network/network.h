#ifndef NETWORK_H
#define NETWORK_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network.h
 * @brief This file contains all code that interacts directly with sockets
 *        under-the-hood.
============================
Usage
============================

SocketContextData: This type represents a socket.
   - To use a socket, call CreateUDPContext or CreateTCPContext with the desired
     parameters
   - To send data over a socket, call send_packet
   - To receive data over a socket, call update and then get_packet to receive packets of a certain type.

WhistPacket: This type represents a packet of information
   - Unique packets of a given type will be given unique IDs. IDs are expected
     to be increasing monotonically, with a gap implying that a packet was lost
   - WhistPackets that were thought to have been sent may not arrive, and
     WhistPackets may arrive out-of-order, in the case of UDP. This will not
     be the case for TCP, however TCP sockets may lose connection if there is a
     problem.
   - A given block of data will, during transmission, be split up into packets
     with the same type and ID, but indicies ranging from 0 to num_indices - 1
   - A missing index implies that a packet was lost
   - A WhistPacket is only guaranteed to have data information from 0 to
     payload_size - 1 data[] occurs at the end of the packet, so extra bytes may
     in-fact point to invalid memory to save space and bandwidth
   - A WhistPacket may be sent twice in the case of packet recovery, but any
     two WhistPackets found that are of the same type and ID will be expected
     to have the same data (To be specific, the Client should never legally send
     two distinct packets with same ID/Type, and neither should the Server, but
if the Client and Server happen to both make a PACKET_MESSAGE packet with ID 1
     they can be different)
   - To reconstruct the original datagram from a sequence of WhistPackets,
     concatenated the data[] streams (From 0 to payload_size - 1) for each index
     from 0 to num_indices - 1

-----
Client
-----

SocketContext context;
create_tcp_socket_context(&context, "10.0.0.5", 5055, 500, 250);

char* msg = "Hello this is a message!";
send_packet(&context, PACKET_MESSAGE, msg, strlen(msg) + 1);

WhistPacket* packet = NULL;
while(packet == NULL) {
    socket_update(context);
    packet = get_packet(&context, PACKET_MESSAGE);
}

LOG_INFO("Response: %s", packet->data); // Will print "Message received!"

free_packet(packet);

destroy_socket_context(&context);

-----
Server
-----

SocketContext context;
create_tcp_socket_context(&context, NULL, 5055, 500, 250);

WhistPacket* packet = NULL;
while(packet == NULL) {
    socket_update(context);
    packet = get_packet(&context, PACKET_MESSAGE);
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

#include <whist/core/whist.h>

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
 * @brief                          The networking state of a socket
 */
// TODO: audio/video seems too specific
typedef struct {
    int fps;
    int bitrate;
    int burst_bitrate;
    double audio_fec_ratio;
    double video_fec_ratio;
    CodecType desired_codec;
} NetworkSettings;

/**
 * @brief                          Packet of data to be sent over a
 *                                 SocketContextData
 */
typedef struct {
    // Metadata
    WhistPacketType type;   // Video, Audio, or Message
    int id;                 // Unique identifier (Two packets with the same type and id, from
                            // the same IP, will be the same)
    int payload_size;       // size of this packet's data[], in bytes

    // Data
    uint8_t data[MAX_PAYLOAD_SIZE];  // data at the end of the struct, with invalid
                                     // bytes beyond payload_size / cipher_len
} WhistPacket;

#define MAX_PACKET_SIZE (sizeof(WhistPacket))
#define PACKET_HEADER_SIZE (sizeof(WhistPacket) - MAX_PAYLOAD_SIZE)
// Real packet size = PACKET_HEADER_SIZE + WhistPacket.payload_size (If
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
typedef enum WhistTOSValue {
    TOS_DSCP_STANDARD_FORWARDING = 0x00,  //<<< Standard Forwarding (Default)
    TOS_DSCP_EXPEDITED_FORWARDING =
        0xb8  //<<< Expedited Forwarding (High Priority, for Video/Audio)
} WhistTOSValue;

typedef struct {
    StunRequestType type;
    StunEntry entry;
} StunRequest;

/**
 * @brief                       Interface describing the available functions
 *                              and socket context of a network protocol
 *
 */
typedef struct {
    // Attributes
    SOCKET* listen_socket;  // listen socket created in advance, typically only used in server.
    void* context;

    // Function table
    bool (*socket_update)(void* context);
    int (*send_packet)(void* context, WhistPacketType type, void* data, int len, int id, bool start_of_stream);
    void* (*get_packet)(void* context, WhistPacketType type);
    void (*free_packet)(void* context, WhistPacket* packet);
    bool (*get_pending_stream_reset)(void* context, WhistPacketType type);
    void (*destroy_socket_context)(void* context);
} SocketContext;

// needed for handshake_private_key
typedef struct SocketContextData SocketContextData;

/*
============================
SocketContext Interface
============================
*/

/**
 * @brief          Keeps the socket alive.
 *                 Failing to call this function periodically
 *                 will cause the connection to drop.
 *
 * @param context  The socket context to update
 * 
 * @returns        True on success,
 *                 False if the connection has been lost
 * 
 * @note           After calling socket_update, you must try to call get_packet,
 *                 otherwise a consecutive call to socket_update
 *                 may overwrite the buffered packet.
 */
bool socket_update(SocketContext* context);

/**
 * @brief                          Given a WhistPacket's type, payload, payload_len, and id,
 *                                 This function will send the WhistPacket over the network.
 *                                 There is no restriction on the size of this packet,
 *                                 it will be fragmented if necessary.
 *
 * @param context                  The socket context
 * @param packet_type              The WhistPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param payload                  A pointer to the payload that is to be sent
 * @param payload_size             The size of the payload
 * @param packet_id                A Packet ID for the packet.
 * @param start_of_stream          Whether or not the client may "skip" to this ID (UDP only)
 *
 * @returns                        Will return -1 on failure,
 *                                 will return 0 on success
 *                                 TODO: Make this either return bool or void
 */
int send_packet(SocketContext* context, WhistPacketType packet_type, void* payload,
                int payload_size, int packet_id, bool start_of_stream);

/**
 * @brief             Receive the next WhistPacket from the given SocketContext of given WhistPacketType
 *
 * @param context     The socket context
 * @param type        Type of packet to pop
 *
 * @returns           A pointer to the next set of data.
 *                    This will point to the same data of the `payload` that was
 *                    passed into `send_packet` on the other side.
 * 
 * @note              socket_update must be called for get_packet to return non-NULL
 */
void* get_packet(SocketContext* context, WhistPacketType type);

/**
 * @brief                          Frees a WhistPacket created by get_packet
 *
 * @param context                  The socket context that created the packet
 * @param packet                   The WhistPacket to free
 */
void free_packet(SocketContext* context, WhistPacket* packet);

/**
 * @brief           Determine whether or not context's stream of packets of the given type
 *                  has been reset during the connection, e.g. due to irrecoverable packet loss
 *
 * @param context   The socket context to read from
 * @param type      The type of data that needs resetting
 *
 * @returns         True if the stream has been reset
 */
bool get_pending_stream_reset(SocketContext context, WhistPacketType type);

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
void whist_init_networking(void);

/**
 * @brief                          Get the most recent network error.
 *
 * @returns                        The network error that most recently occured,
 *                                 through WSAGetLastError on Windows or errno
 *                                 on Linux
 */
int get_last_network_error(void);

/**
 * @brief                          Get the size of a WhistPacket
 *
 * @param packet                   The packet to get the size of
 *
 * @returns                        The size of the packet, or -1 on error
 */
int get_packet_size(WhistPacket* packet);

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
void set_tos(SOCKET socket, WhistTOSValue tos);

/**
 * @brief                          Perform a UDP socket() syscall and set fds to
 *                                 use flag FD_CLOEXEC if needed. This prevents
 *                                 child processes from holding onto the socket,
 *                                 preventing the socket's closure.
 *
 * @returns                        The socket file descriptor, -1 on failure
 */
SOCKET socketp_udp(void);

/**
 * @brief                          Perform socket syscalls and set fds to
 *                                 use flag FD_CLOEXEC
 *
 * @returns                        The socket file descriptor, -1 on failure
 */

/**
 * @brief                          Given a socket, this will perform a private key
 *                                 handshake with the other side of the connection,
 *                                 to verify their authenticity
 *
 * @param socket                   The socket to handshake over
 * @param connection_timeout_ms    How long to try to handshake before giving up
 * @param private_key              The private key to use for the handshake
 * 
 * @returns                        True on success, False on failure
 * 
 * @note                           The timeout for the socket may be overwritten
 *                                 after this call, please call set_timeout to restore it
 */
bool handshake_private_key(SOCKET socket, int connection_timeout_ms, const void* private_key);

/**
 * @brief Call recv() while ignoring EINTR returns.
 *
 * The function signature is identical to the host recv() call.
 */
ssize_t recv_no_intr(int sockfd, void* buf, size_t len, int flags);

/**
 * @brief Call recvfrom() while ignoring EINTR returns.
 *
 * The function signature is identical to the host recv() call.
 */
ssize_t recvfrom_no_intr(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr,
                         socklen_t* addrlen);

// TODO: Move
#include <whist/network/tcp.h>
#include <whist/network/udp.h>

#endif  // NETWORK_H
