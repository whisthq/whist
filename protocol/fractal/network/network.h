#ifndef NETWORK_H
#define NETWORK_H

/*

This file contains all code that interacts directly with sockets under-the-hood.

Usage -

SocketContext: This type represents a socket.
   To use a socket, call CreateUDPContext or CreateTCPContext with the desired parameters
   To send data over a socket, call SendTCPPacket or SendUDPPacket
   To receive data over a socket, call ReadTCPPacket or ReadUDPPacket
   If there is belief that a packet wasn't sent, you can call ReplayPacket to send a packet twice

*/

// *** begin includes ***

#if defined(_WIN32)
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "../core/fractal.h"

// *** end includes ***

// *** begin defines ***

#define STUN_IP "52.22.246.213"
#define STUN_PORT 48800

#define LARGEST_TCP_PACKET 10000000
#define MAX_PAYLOAD_SIZE 1285

// windows socklen
#if defined(_WIN32)
#undef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
#undef EINPROGRESS
#define EINPROGRESS WSAEWOULDBLOCK
#define socklen_t int
#define FRACTAL_IOCTL_SOCKET ioctlsocket
#define FRACTAL_CLOSE_SOCKET closesocket
#else
#define SOCKET int
#define closesocket close
#define FRACTAL_IOCTL_SOCKET ioctl
#define FRACTAL_CLOSE_SOCKET close
#endif

#define TCP_SEGMENT_SIZE 1024

// *** end defines ***

// *** begin typedefs ***

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

typedef enum FractalPacketType
{
    PACKET_AUDIO,
    PACKET_VIDEO,
    PACKET_MESSAGE,
} FractalPacketType;

// Real Packet Size = sizeof(FractalPacket) - sizeof(FractalPacket.data) +
// RTPPacket.payload_size
typedef struct FractalPacket {
    // hash at the beginning of the struct, which is the hash of the rest of the
    // packet
    char hash[16];
    // hash is a signature for everything below this line
    int cipher_len;
    char iv[16];
    // Everything below this line gets encrypted
    FractalPacketType type;
    int id;
    short index;
    short num_indices;
    int payload_size;
    bool is_a_nack;
    // data at the end of the struct, in the case of a truncated packet
    uint8_t data[MAX_PAYLOAD_SIZE];
    // The encrypted packet could overflow
    uint8_t overflow[16];
} FractalPacket;

#define MAX_PACKET_SIZE (sizeof(FractalPacket))
#define PACKET_HEADER_SIZE (sizeof(FractalPacket) - MAX_PAYLOAD_SIZE - 16)

// *** end typedefs ***

// *** begin functions ***

/*
@brief This will set the socket s to have timeout timeout_ms. Use 0 to have a non-blocking socket, and -1 for an indefinitely blocking socket

@returns The network error that most recently occured, through WSAGetLastError on windows or errno on linux
*/
int GetLastNetworkError();

/*
@brief                          Initialize a UDP/TCP connection between a server and a client

@param context                  The socket context that will be initialized
@param destination              The server IP address to connect to. Passing NULL will wait for another client to connect to the socket
@param port                     The port to connect to. This will be a real port if USING_STUN is false, and it will be a virtual port if USING_STUN is true. (The real port will be some randomly chosen port if USING_STUN is true)
@param recvfrom_timeout_s       The timeout that the socketcontext will use after being initialized
@param connection_timeout_ms    The timeout that will be used when attempting to connect. The handshake sends a few packets back and forth, so the upper bound of how long CreateXContext will take is some small constant times connection_timeout_ms

@returns                        Will return -1 on failure, will return 0 on success
*/
int CreateUDPContext(SocketContext* context, char* destination,
                     int port, int recvfrom_timeout_s, int connection_timeout_ms );
int CreateTCPContext(SocketContext* context, char* destination,
                     int port, int recvfrom_timeout_s, int connection_timeout_ms );

/*
@brief                          This will send a FractalPacket over TCP to the SocketContext context.
                                A FractalPacketType is also provided to describe the packet

@param context                  The socket context
@param type                     The FractalPacketType, either VIDEO, AUDIO, or MESSAGE
@param data                     A pointer to the data to be sent
@param len                      The nubmer of bytes to send

@returns                        Will return -1 on failure, will return 0 on success
*/
int SendTCPPacket( SocketContext* context, FractalPacketType type,
                   void* data, int len );

/*
@brief                          This will send a FractalPacket over UDP to the SocketContext context.
                                A FractalPacketType is also provided to the receiving end.

@param context                  The socket context
@param type                     The FractalPacketType, either VIDEO, AUDIO, or MESSAGE
@param data                     A pointer to the data to be sent
@param len                      The nubmer of bytes to send
@param id                       An ID for the UDP data.
@param burst_bitrate            The maximum bitrate that packets will be sent over. -1 will imply sending as fast as possible
@param packet_buffer            An array of RTPPacket's, each sub-packet of the UDPPacket will be stored in packet_buffer[i]
@param packet_len_buffer        An array of int's, defining the length of each sub-packet located in packet_buffer[i]

@returns                        Will return -1 on failure, will return 0 on success
*/
int SendUDPPacket( SocketContext* context, FractalPacketType type,
                   void* data, int len, int id, int burst_bitrate, FractalPacket* packet_buffer, int* packet_len_buffer );

/*
@brief                          Replay the sending of a packet that has already been sent by the network protocol. (Via a packet_buffer write from SendUDPPacket)

@param context                  The socket context
@param packet                   The packet to resend
@param len                      The length of the packet to resend

@returns                        Will return -1 on failure, will return 0 on success
*/
int ReplayPacket( SocketContext* context, FractalPacket* packet, size_t len );

/*
@brief                          Send a 0-length packet over the socket. Used to keep-alive over NATs, and to check on the validity of the socket

@param context                  The socket context

@returns                        Will return -1 on failure, will return 0 on success
                                Failure implies that the socket is broken or the TCP connection has ended, use GetLastNetworkError() to learn more about the error
*/
int ack( SocketContext* context );

void ClearReadingTCP();


/*
@brief                          Receive a FractalPacket from a SocketContext, if any such packet exists

@param context                  The socket context

@returns                        A pointer to the FractalPacket on success, NULL on failure
*/
FractalPacket* ReadTCPPacket(SocketContext* context);
FractalPacket* ReadUDPPacket( SocketContext* context );

// @brief sends a JSON POST request to the Fractal webservers
// @details authenticate the user and return the credentials
bool sendJSONPost(char* host_s, char* path, char* jsonObj);

// *** end functions ***

#endif  // NETWORK_H