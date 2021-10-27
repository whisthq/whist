#ifndef NETWORK_H
#define NETWORK_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file network.h
 * @brief This file contains all code that interacts directly with sockets
 *        under-the-hood.
============================
Usage
============================

SocketAttributes: This type represents a socket.
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

SocketAttributes context;
CreateTCPContext(&context, "10.0.0.5", 5055, 500, 250);

char* msg = "Hello this is a message!";
SendTCPPacket(&context, PACKET_MESSAGE, msg, strlen(msg);

-----
Server
-----

SocketAttributes context;
CreateTCPContext(&context, NULL, 5055, 500, 250);

FractalPacket* packet = NULL;
while(!packet) {
  packet = ReadTCPPacket(context);
}

LOG_INFO("MESSAGE: %s", packet->data); // Will print "Hello this is a message!"
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

#include <fractal/core/fractal.h>
#include <fractal/network/throttle.h>

/*
============================
Defines
============================
*/

#if defined(_WIN32)
#define FRACTAL_ETIMEDOUT WSAETIMEDOUT
#define FRACTAL_EWOULDBLOCK WSAEWOULDBLOCK
#define FRACTAL_EAGAIN WSAEWOULDBLOCK
#define FRACTAL_EINPROGRESS WSAEWOULDBLOCK
#define socklen_t int
#define FRACTAL_IOCTL_SOCKET ioctlsocket
#define FRACTAL_CLOSE_SOCKET closesocket
#else
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#define FRACTAL_IOCTL_SOCKET ioctl
#define FRACTAL_CLOSE_SOCKET close
#define FRACTAL_ETIMEDOUT ETIMEDOUT
#define FRACTAL_EWOULDBLOCK EWOULDBLOCK
#define FRACTAL_EAGAIN EAGAIN
#define FRACTAL_EINPROGRESS EINPROGRESS
#endif

#define STUN_IP "0.0.0.0"
#define STUN_PORT 48800

// Note that both the Windows and Linux versions use 2 as the second argument
// to indicate shutting down both ends of the socket
#define FRACTAL_SHUTDOWN_SOCKET(s) shutdown(s, 2)

/*
============================
Constants
============================
*/

#define MAX_PAYLOAD_SIZE 1285

/*
============================
Custom types
============================
*/

typedef struct SocketAttributes {
    bool is_server;
    bool is_tcp;
    bool udp_is_connected;
    int timeout;
    SOCKET socket;
    struct sockaddr_in addr;
    int ack;
    FractalMutex mutex;
    char binary_aes_private_key[16];
    // Used for reading TCP packets
    int reading_packet_len;
    DynamicBuffer* encrypted_tcp_packet_buffer;
    NetworkThrottleContext* network_throttler;
} SocketAttributes;

// TODO: Unique PRIVATE_KEY for every session, so that old packets can't be
// replayed
// TODO: INC integer that must not be used twice

/**
 * @brief                          Data packet description
 */
typedef enum FractalPacketType {
    PACKET_AUDIO,
    PACKET_VIDEO,
    PACKET_MESSAGE,
} FractalPacketType;

/**
 * @brief                          Packet of data to be sent over a
 *                                 SocketAttributes
 */
typedef struct FractalPacket {
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

typedef struct {
    unsigned int ip;
    unsigned short private_port;
    unsigned short public_port;
} StunEntry;

typedef enum StunRequestType { ASK_INFO, POST_INFO } StunRequestType;

typedef struct {
    StunRequestType type;
    StunEntry entry;
} StunRequest;

/**
 * @brief                       Interface describing the avaliable functions
 *                              and socket context of a network protocol
 *
 */
typedef struct SocketContext {
    // Attributes
    void* context;

    // Functions common to all network connections
    int (*ack)(SocketAttributes* context);
    FractalPacket* (*read_packet)(SocketAttributes* context, bool should_recvp);

    int (*send_packet_from_payload)(SocketAttributes* context, FractalPacketType type, void* data,
                                    int len, int id);  // id only valid in UDP contexts
    int (*send_packet)(SocketAttributes* context, FractalPacket* packet, size_t packet_size);
    void (*free_packet)(FractalPacket* packet);  // Only Non-NULL in TCP.
    int (*write_payload_to_packets)(uint8_t* payload, size_t payload_size, int payload_id,
                             FractalPacketType packet_type, FractalPacket* packet_buffer,
                             size_t packet_buffer_length);
} SocketContext;

#define MAX_PACKET_SIZE (sizeof(FractalPacket))
#define PACKET_HEADER_SIZE (sizeof(FractalPacket) - MAX_PAYLOAD_SIZE - 16)
// Real packet size = PACKET_HEADER_SIZE + FractalPacket.payload_size (If
// Unencrypted)
//                  = PACKET_HEADER_SIZE + cipher_len (If Encrypted)

/*
============================
Public Functions
============================
*/

/*
 * @brief                          Initialize networking system
 *                                 Must be called before calling any other function in this file
 */
void init_networking();

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
 * @brief                          Send a 0-length packet over the socket. Used
 *                                 to keep-alive over NATs, and to check on the
 *                                 validity of the socket
 *
 * @param context                  The socket context
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success Failure implies that the socket is
 *                                 broken or the TCP connection has ended, use
 *                                 GetLastNetworkError() to learn more about the
 *                                 error
 */
int ack(SocketContext* context);

FractalPacket* read_packet(SocketContext* context, bool should_recvp);
int send_packet_from_payload(SocketContext* context, FractalPacketType type, void* data,
                                int len, int id);  // id only valid in UDP contexts
int send_packet(SocketContext* context, FractalPacket* packet, size_t packet_size);
void free_packet(SocketContext* context, FractalPacket* packet);  // Only Non-NULL in TCP.
int write_payload_to_packets(SocketContext* context, uint8_t* payload, size_t payload_size, int payload_id,
                          FractalPacketType packet_type, FractalPacket* packet_buffer,
                          size_t packet_buffer_length);

void destroy_socket_context(SocketContext* context);



typedef struct {
    char iv[16];
    char signature[32];
} PrivateKeyData;

typedef struct {
    char iv[16];
    char private_key[16];
} SignatureData;

/*
@brief                          This will set `socket` to have timeout
                                timeout_ms.

@param socket                   The SOCKET to be configured
@param timeout_ms               The maximum amount of time that all recv/send
                                calls will take for that socket (0 to return
                                immediately, -1 to never return). Set 0 to have
                                a non-blocking socket, and -1 for an indefinitely
                                blocking socket.
*/
void set_timeout(SOCKET socket, int timeout_ms);

/*
@brief                          Perform socket syscalls and set fds to
                                use flag FD_CLOEXEC

@returns                        The socket file descriptor, -1 on failure
*/
SOCKET socketp_tcp();
SOCKET socketp_udp();

/*
@brief                          This will prepare the private key data

@param priv_key_data            The private key data buffer
*/
void prepare_private_key_request(PrivateKeyData* priv_key_data);

/*
@brief                          This will sign the other connection's private key data

@param priv_key_data            The private key data buffer
@param recv_size                The length of the buffer
@param private_key              The private key

@returns                        True if the verification succeeds, false if it fails
*/
bool sign_private_key(PrivateKeyData* priv_key_data, int recv_size, void* private_key);

/*
@brief                          This will verify the given private key

@param our_priv_key_data        The private key data buffer
@param our_signed_priv_key_data The signed private key data buffer
@param recv_size                The length of the buffer
@param private_key              The private key

@returns                        True if the verification succeeds, false if it fails
*/
bool confirm_private_key(PrivateKeyData* our_priv_key_data,
                         PrivateKeyData* our_signed_priv_key_data, int recv_size,
                         void* private_key);

bool handshake_private_key(SocketAttributes* context);

/**
 * @brief                          This will send or receive data over a socket
 *
 * @param context                  The socket context to be used
 * @param buf                      The buffer to read or write to
 * @param len                      The length of the buffer to send over the socket
                                   Or, the maximum number of bytes that can be read
 *                                 from the socket

 * @returns                        The number of bytes that have been read or
 *                                 written to or from the buffer
 */
int recvp(SocketAttributes* context, void* buf, int len);
int sendp(SocketAttributes* context, void* buf, int len);

#include <fractal/network/tcp.h>
#include <fractal/network/udp.h>

#endif  // NETWORK_H
