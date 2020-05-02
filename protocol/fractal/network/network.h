#ifndef NETWORK_H
#define NETWORK_H

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
} SocketContext;

/// @brief Connection origin.
/// @details Passed to CreateTCPContext and CreateUDPContext
typedef enum FractalConnectionOrigin {
    ORIGIN_CLIENT = 1,  ///< Connection from a client.
    ORIGIN_SERVER = 2   ///< Connection from a server.
} FractalConnectionOrigin;

// TODO: Unique PRIVATE_KEY for every session, so that old packets can't be
// replayed
// TODO: INC integer that must not be used twice

typedef enum FractalPacketType
{
    PACKET_AUDIO,
    PACKET_VIDEO,
    PACKET_MESSAGE,
} FractalPacketType;

// Real Packet Size = sizeof(RTPPacket) - sizeof(RTPPacket.data) +
// RTPPacket.payload_size
struct RTPPacket {
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
};

#define MAX_PACKET_SIZE (sizeof(struct RTPPacket))
#define PACKET_HEADER_SIZE (sizeof(struct RTPPacket) - MAX_PAYLOAD_SIZE - 16)

// *** end typedefs ***

// *** begin functions ***

int GetLastNetworkError();

void set_timeout(SOCKET s, int timeout_ms);


int SendTCPPacket( struct SocketContext* context, FractalPacketType type,
                   void* data, int len );
int CreateUDPContext(struct SocketContext* context,
                     FractalConnectionOrigin origin, char* destination,
                     int port, int recvfrom_timeout_s, int stun_timeout_ms);
int CreateTCPContext(struct SocketContext* context,
                     FractalConnectionOrigin origin, char* destination,
                     int port, int recvfrom_timeout_s, int stun_timeout_ms);
int recvp(struct SocketContext* context, void* buf, int len);
int sendp(struct SocketContext* context, void* buf, int len);

void ClearReadingTCP();
void* TryReadingTCPPacket(struct SocketContext* context);

// @brief sends a JSON POST request to the Fractal webservers
// @details authenticate the user and return the credentials
bool sendJSONPost(char* host_s, char* path, char* jsonObj);

// *** end functions ***

#endif  // NETWORK_H