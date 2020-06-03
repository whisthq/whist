#ifndef NETWORK_H
#define NETWORK_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.h
 * @brief This file contains all code that interacts directly with sockets
 *        under-the-hood.
============================
Usage
============================

SocketContext: This type represents a socket.
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
two distinct packets with same ID/Type, and neither should the Server, but if
     the Client and Server happen to both make a PACKET_MESSAGE packet with ID 1
     they can be different)
   - To reconstruct the original datagram from a sequence of FractalPackets,
     concatenated the data[] streams (From 0 to payload_size - 1) for each index
     from 0 to num_indices - 1

-----
Client
-----

SocketContext context;
CreateTCPContext(&context, "10.0.0.5", 5055, 500, 250);

char* msg = "Hello this is a message!";
SendTCPPacket(&context, PACKET_MESSAGE, msg, strlen(msg);

-----
Server
-----

SocketContext context;
CreateTCPContext(&context, NULL, 5055, 500, 250);

FractalPacket* packet = NULL;
while(!packet) {
  packet = ReadTCPPacket(context);
}

printf("MESSAGE: %s\n", packet->data); // Will print "Hello this is a message!"
*/

/*
============================
Includes
============================
*/

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

#include "../core/fractal.h"

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
#define closesocket close
#define FRACTAL_IOCTL_SOCKET ioctl
#define FRACTAL_CLOSE_SOCKET close
#define FRACTAL_ETIMEDOUT ETIMEDOUT
#define FRACTAL_EWOULDBLOCK EWOULDBLOCK
#define FRACTAL_EAGAIN EAGAIN
#define FRACTAL_EINPROGRESS EINPROGRESS
#endif

/*
============================
Constants
============================
*/

#define LARGEST_TCP_PACKET 10000000
#define MAX_PAYLOAD_SIZE 1285
#define TCP_SEGMENT_SIZE 1024

/*
============================
Custom types
============================
*/

typedef struct SocketContext {
    bool is_server;
    bool is_tcp;
    SOCKET s;
    struct sockaddr_in addr;
    int ack;
    SDL_mutex* mutex;
} SocketContext;

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
 *                                 SocketContext
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
    int id;  // Unique identifier (Two packets with the same type and id, from
             // the same IP, will be the same)
    short index;        // Handle separation of large datagrams
    short num_indices;  // The total datagram consists of data packets with
                        // indices from 0 to payload_size - 1
    int payload_size;   // size of data[] that is of interest
    bool is_a_nack;     // True if this is a replay'ed packet

    // Data
    uint8_t
        data[MAX_PAYLOAD_SIZE];  // data at the end of the struct, with invalid
                                 // bytes beyond payload_size / cipher_len
    uint8_t overflow[16];  // The maximum cipher_len is MAX_PAYLOAD_SIZE + 16,
                           // as the encrypted packet might be slightly larger
                           // than the unencrypted packet
} FractalPacket;

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

/**
 * @brief                          This will set the socket s to have timeout
 *                                 timeout_ms. Use 0 to have a non-blocking
 *                                 socket, and -1 for an indefinitely blocking
 * socket
 *
 * @returns                        The network error that most recently occured,
 *                                 through WSAGetLastError on Windows or errno
 *                                 on Linux
 */
int GetLastNetworkError();

/**
 * @brief                          Initialize a UDP/TCP connection between a
 *                                 server and a client
 *
 * @param context                  The socket context that will be initialized
 * @param destination              The server IP address to connect to. Passing
 *                                 NULL will wait for another client to connect
 *                                 to the socket
 * @param port                     The port to connect to. This will be a real
 *                                 port if USING_STUN is false, and it will be a
 *                                 virtual port if USING_STUN is
 *                                 true; (The real port will be some randomly
 *                                 chosen port if USING_STUN is true)
 * @param recvfrom_timeout_s       The timeout that the socketcontext will use
 *                                 after being initialized
 * @param connection_timeout_ms    The timeout that will be used when attempting
 *                                 to connect. The handshake sends a few packets
 *                                 back and forth, so the upper
 *                                 bound of how long CreateXContext will take is
 *                                 some small constant times
 *                                 connection_timeout_ms
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int CreateUDPContext(SocketContext* context, char* destination, int port,
                     int recvfrom_timeout_s, int connection_timeout_ms,
                     bool using_stun);
int CreateTCPContext(SocketContext* context, char* destination, int port,
                     int recvfrom_timeout_s, int connection_timeout_ms,
                     bool using_stun);

/**
 * @brief                          This will send a FractalPacket over TCP to
 *                                 the SocketContext context. A
 *                                 FractalPacketType is also provided to
 *                                 describe the packet
 *
 * @param context                  The socket context
 * @param type                     The FractalPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The nubmer of bytes to send
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int SendTCPPacket(SocketContext* context, FractalPacketType type, void* data,
                  int len);

/**
 * @brief                          This will send a FractalPacket over UDP to
 *                                 the SocketContext context. A
 *                                 FractalPacketType is also provided to the
 *                                 receiving end
 *
 * @param context                  The socket context
 * @param type                     The FractalPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The nubmer of bytes to send
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
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int SendUDPPacket(SocketContext* context, FractalPacketType type, void* data,
                  int len, int id, int burst_bitrate,
                  FractalPacket* packet_buffer, int* packet_len_buffer);

/**
 * @brief                          Replay the sending of a packet that has
 *                                 already been sent by the network protocol.
 *                                 (Via a packet_buffer write from
 *                                 SendUDPPacket)
 *
 * @param context                  The socket context
 * @param packet                   The packet to resend
 * @param len                      The length of the packet to resend
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int ReplayPacket(SocketContext* context, FractalPacket* packet, size_t len);

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
int Ack(SocketContext* context);

/**
 * @brief                          Receive a FractalPacket from a SocketContext,
 *                                 if any such packet exists
 *
 * @param context                  The socket context
 *
 * @returns                        A pointer to the FractalPacket on success,
 *                                 NULL on failure
 */
FractalPacket* ReadTCPPacket(SocketContext* context);
FractalPacket* ReadUDPPacket(SocketContext* context);

/**
 * @brief                          Sends a JSON POST request to the Fractal
 *                                 webservers
 *
 * @param host_s                   The hostname IP address
 * @param path                     The /path/to/the/endpoint
 * @param jsonObj                  A string consisting of the JSON-complient
 *                                 datastream to send to the webserver
 *
 * @returns                        Will return false on failure, will return
 *                                 true on success Failure implies that the
 *                                 socket is broken or the TCP connection has
 *                                 ended, use GetLastNetworkError() to learn
 *                                 more about the error
 */
bool SendJSONPost(char* host_s, char* path, char* jsonObj);

/**
 * @brief                          Sends a JSON GET request to the Fractal
 *                                 webservers
 *
 * @param host_s                   The hostname IP address
 * @param path                     The /path/to/the/endpoint
 * @param json_res                 The buffer in which to store the JSON
 *                                 response
 * @param json_res_size            The size of the response buffer
 *
 * @returns                        Will return false on failure, will return
 *                                 true on success Failure implies that the
 *                                 socket is broken or the TCP connection has
 *                                 ended, use GetLastNetworkError() to learn
 *                                 more about the error
 */
bool SendJSONGet(char* host_s, char* path, char* json_res,
                 size_t json_res_size);

#endif  // NETWORK_H