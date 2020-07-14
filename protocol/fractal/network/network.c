/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.c
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

#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, need to
                                         // be at the very top
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "network.h"

#include <stdio.h>

#include "../utils/aes.h"

#define STUN_IP "52.5.240.234"
#define STUN_PORT 48800

#define UNUSED(x) (void)(x)
#define BITS_IN_BYTE 8.0
#define MS_IN_SECOND 1000

/*
============================
Private Custom Types
============================
*/

typedef struct {
    unsigned int ip;
    unsigned short private_port;
    unsigned short public_port;
} stun_entry_t;

typedef enum stun_request_type { ASK_INFO, POST_INFO } stun_request_type_t;

typedef struct {
    stun_request_type_t type;
    stun_entry_t entry;
} stun_request_t;

typedef struct {
    char iv[16];
    char signature[32];
} private_key_data_t;

/*
============================
Private Functions
============================
*/

/*
@brief                          This will set the socket s to have timeout
timeout_ms.

@param s                        The SOCKET to be configured
@param timeout_ms               The maximum amount of time that all recv/send
calls will take for that socket (0 to return immediately, -1 to never return)
                                Set 0 to have a non-blocking socket, and -1 for
an indefinitely blocking socket
*/
void set_timeout(SOCKET s, int timeout_ms);

/*
@brief                          This will send or receive data over a socket

@param s                        The SOCKET to be used
@param buf                      The buffer to read or write to
@param len                      The length of the buffer to send over the socket
                                Or, the maximum number of bytes that can be read
from the socket

@returns                        The number of bytes that have been read or
written to or from the buffer
*/
int recvp(SocketContext *context, void *buf, int len);
int sendp(SocketContext *context, void *buf, int len);

/*
@brief                          This will initialize and clear the TCP reader
buffer. TCP data will have be read into this buffer, and will only return a
packet when the entire packet is received

@param context                  The socket context
*/
void ClearReadingTCP(SocketContext *context);

/*
@brief                          This will prepare the private key data

@param priv_key_data            The private key data buffer
*/
void preparePrivateKeyRequest(private_key_data_t *priv_key_data);

/*
@brief                          This will sign the other connection's private key data

@param priv_key_data            The private key data buffer
@param recv_size                The length of the buffer
@param private_key              The private key

@returns                        True if the verification succeeds, false if it fails
*/
bool signPrivateKey(private_key_data_t *priv_key_data, int recv_size, void *private_key);

/*
@brief                          This will verify the given private key

@param our_priv_key_data        The private key data buffer
@param our_signed_priv_key_data The signed private key data buffer
@param recv_size                The length of the buffer
@param private_key              The private key

@returns                        True if the verification succeeds, false if it fails
*/
bool confirmPrivateKey(private_key_data_t *our_priv_key_data,
                       private_key_data_t *our_signed_priv_key_data, int recv_size,
                       void *private_key);

/*
============================
Public Function Implementations
============================
*/

int GetLastNetworkError() {
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool handshakePrivateKey(SocketContext *context) {
    private_key_data_t our_priv_key_data;
    private_key_data_t our_signed_priv_key_data;
    private_key_data_t their_priv_key_data;
    int recv_size;
    int slen = sizeof(context->addr);

    // Open up the port
    if( sendp( context, NULL, 0 ) < 0 )
    {
        LOG_ERROR( "sendp(3) failed! Could not open up port! %d", GetLastNetworkError() );
        return false;
    }
    SDL_Delay( 150 );

    // Generate and send private key request data
    preparePrivateKeyRequest( &our_priv_key_data );
    if( sendp( context, &our_priv_key_data, sizeof( our_priv_key_data ) ) < 0 )
    {
        LOG_ERROR( "sendp(3) failed! Could not send private key request data! %d", GetLastNetworkError() );
        return false;
    }
    SDL_Delay(150);

    // Receive, sign, and send back their private key request data
    while( (recv_size = recvfrom( context->s, (char*)&their_priv_key_data, sizeof( their_priv_key_data ), 0, (struct sockaddr*)(&context->addr), &slen )) == 0 );
    if( !signPrivateKey( &their_priv_key_data, recv_size, context->aes_private_key ) )
    {
        LOG_ERROR( "signPrivateKey failed!" );
        return false;
    }
    if( sendp( context, &their_priv_key_data, sizeof( their_priv_key_data ) ) < 0 )
    {
        LOG_ERROR( "sendp(3) failed! Could not send signed private key data! %d", GetLastNetworkError() );
        return false;
    }
    SDL_Delay(150);

    // Wait for and verify their signed private key request data
    recv_size = recvp(context, &our_signed_priv_key_data, sizeof(their_priv_key_data));
    if (!confirmPrivateKey(&our_priv_key_data, &our_signed_priv_key_data, recv_size,
                           context->aes_private_key)) {
        LOG_ERROR("Could not confirmPrivateKey!");
        return false;
    } else {
        return true;
    }
}

#define LARGEST_TCP_PACKET 10000000
#define PACKET_ENCRYPTION_PADDING \
    16  // encryption can make packets a bit bigger, this pads to avoid overflow
#define LARGEST_ENCRYPTED_TCP_PACKET (sizeof(int) + LARGEST_TCP_PACKET + PACKET_ENCRYPTION_PADDING)

int SendTCPPacket(SocketContext *context, FractalPacketType type, void *data, int len) {
    // Verify packet size can fit
    if (PACKET_HEADER_SIZE + (unsigned int)len > LARGEST_TCP_PACKET) {
        LOG_WARNING("Packet too large!");
        return -1;
    }
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    static char packet_buffer[LARGEST_TCP_PACKET];
    static char encrypted_packet_buffer[LARGEST_ENCRYPTED_TCP_PACKET];

    FractalPacket *packet = (FractalPacket *)packet_buffer;

    // Contruct packet metadata
    packet->id = -1;
    packet->type = type;
    packet->index = 0;
    packet->payload_size = len;
    packet->num_indices = 1;
    packet->is_a_nack = false;

    // Copy packet data
    memcpy(packet->data, data, len);

    // Encrypt the packet using aes encryption
    int unencrypted_len = PACKET_HEADER_SIZE + packet->payload_size;
    int encrypted_len = encrypt_packet(packet, unencrypted_len,
                                       (FractalPacket *)(sizeof(int) + encrypted_packet_buffer),
                                       (unsigned char *)context->aes_private_key);

    // Pass the length of the packet as the first byte
    *((int *)encrypted_packet_buffer) = encrypted_len;

    // Send the packet
    LOG_INFO("Sending TCP Packet of length %d\n", encrypted_len);
    bool failed = false;
    if (sendp(context, encrypted_packet_buffer, sizeof(int) + encrypted_len) < 0) {
        LOG_WARNING("Failed to send packet!");
        failed = true;
    }

    // Return success code
    return failed ? -1 : 0;
}

int SendUDPPacket(SocketContext *context, FractalPacketType type, void *data, int len, int id,
                  int burst_bitrate, FractalPacket *packet_buffer, int *packet_len_buffer) {
    if (id <= 0) {
        LOG_WARNING("IDs must be positive!");
        return -1;
    }
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    int payload_size;
    int curr_index = 0, i = 0;

    int num_indices = len / MAX_PAYLOAD_SIZE + (len % MAX_PAYLOAD_SIZE == 0 ? 0 : 1);

    double max_bytes_per_second = burst_bitrate / BITS_IN_BYTE;

    /*
    if (type == PACKET_AUDIO) {
        static int ddata = 0;
        static clock last_timer;
        if( ddata == 0 )
        {
            StartTimer( &last_timer );
        }
        ddata += len;
        GetTimer( last_timer );
        if( GetTimer( last_timer ) > 5.0 )
        {
            mprintf( "AUDIO BANDWIDTH: %f kbps", 8 * ddata / GetTimer(
    last_timer ) / 1024 ); ddata = 0;
        }
        // mprintf("Video ID %d (Packets: %d)\n", id, num_indices);
    }
    */

    clock packet_timer;
    StartTimer(&packet_timer);

    while (curr_index < len) {
        // Delay distribution of packets as needed
        while (burst_bitrate > 0 &&
               curr_index - 5000 > GetTimer(packet_timer) * max_bytes_per_second) {
            SDL_Delay(1);
        }

        // local packet and len for when nack buffer isn't needed
        FractalPacket l_packet = {0};
        int l_len = 0;

        int *packet_len = &l_len;
        FractalPacket *packet = &l_packet;

        // Based on packet type, the packet to one of the buffers to serve later
        // nacks
        if (packet_buffer) {
            packet = &packet_buffer[i];
        }

        if (packet_len_buffer) {
            packet_len = &packet_len_buffer[i];
        }

        payload_size = min(MAX_PAYLOAD_SIZE, (len - curr_index));

        // Construct packet
        packet->type = type;
        memcpy(packet->data, (uint8_t *)data + curr_index, payload_size);
        packet->index = (short)i;
        packet->payload_size = payload_size;
        packet->id = id;
        packet->num_indices = (short)num_indices;
        packet->is_a_nack = false;
        int packet_size = PACKET_HEADER_SIZE + packet->payload_size;

        // Save the len to nack buffer lens
        *packet_len = packet_size;

        // Encrypt the packet with AES
        FractalPacket encrypted_packet;
        int encrypt_len = encrypt_packet(packet, packet_size, &encrypted_packet,
                                         (unsigned char *)context->aes_private_key);

        // Send it off
        SDL_LockMutex(context->mutex);
        int sent_size = sendp(context, &encrypted_packet, encrypt_len);
        SDL_UnlockMutex(context->mutex);

        if (sent_size < 0) {
            int error = GetLastNetworkError();
            mprintf("Unexpected Packet Error: %d\n", error);
            return -1;
        }

        i++;
        curr_index += payload_size;
    }

    // LOG_INFO( "Packet Time: %f\n", GetTimer( packet_timer ) );

    return 0;
}

int ReplayPacket(SocketContext *context, FractalPacket *packet, size_t len) {
    if (len > sizeof(FractalPacket)) {
        LOG_WARNING("Len too long!\n");
        return -1;
    }
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    if (packet == NULL) {
        LOG_WARNING("packet is NULL");
        return -1;
    }

    packet->is_a_nack = true;

    FractalPacket encrypted_packet;
    int encrypt_len = encrypt_packet(packet, (int)len, &encrypted_packet,
                                     (unsigned char *)context->aes_private_key);

    SDL_LockMutex(context->mutex);
    int sent_size = sendp(context, &encrypted_packet, encrypt_len);
    SDL_UnlockMutex(context->mutex);

    if (sent_size < 0) {
        mprintf("Could not replay packet!\n");
        return -1;
    }

    return 0;
}

int recvp(SocketContext *context, void *buf, int len) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    return recv(context->s, buf, len, 0);
}

int sendp(SocketContext *context, void *buf, int len) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    if (len != 0 && buf == NULL) {
        LOG_ERROR("Passed non zero length and a NULL pointer to sendto");
        return -1;
    }
    // cppcheck-suppress nullPointer
    return sendto(context->s, buf, len, 0, (struct sockaddr *)(&context->addr),
                  sizeof(context->addr));
}

int Ack(SocketContext *context) { return sendp(context, NULL, 0); }

bool tcp_connect(SOCKET s, struct sockaddr_in addr, int timeout_ms) {
    // Connect to TCP server
    int ret;
    set_timeout(s, 0);
    if ((ret = connect(s, (struct sockaddr *)(&addr), sizeof(addr))) < 0) {
        bool worked = GetLastNetworkError() == FRACTAL_EINPROGRESS;

        if (!worked) {
            LOG_WARNING(
                "Could not connect() over TCP to server: Returned %d, Error "
                "Code %d\n",
                ret, GetLastNetworkError());
            closesocket(s);
            return false;
        }
    }

    // Select connection
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / MS_IN_SECOND;
    tv.tv_usec = (timeout_ms % MS_IN_SECOND) * MS_IN_SECOND;
    if ((ret = select((int)s + 1, NULL, &set, NULL, &tv)) <= 0) {
        LOG_WARNING(
            "Could not select() over TCP to server: Returned %d, Error Code "
            "%d\n",
            ret, GetLastNetworkError());
        closesocket(s);
        return false;
    }

    set_timeout(s, timeout_ms);
    return true;
}

FractalPacket *ReadUDPPacket(SocketContext *context) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return NULL;
    }

    // Wait to receive packet over TCP, until timing out
    FractalPacket encrypted_packet;
    int encrypted_len = recvp(context, &encrypted_packet, sizeof(encrypted_packet));

    static FractalPacket decrypted_packet;

    // If the packet was successfully received, then decrypt it
    if (encrypted_len > 0) {
        int decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len, &decrypted_packet,
                                           (unsigned char *)context->aes_private_key);

        // If there was an issue decrypting it, post warning and then
        // ignore the problem
        if (decrypted_len < 0) {
            if (encrypted_len == sizeof(stun_entry_t)) {
                stun_entry_t *e;
                e = (void *)&encrypted_packet;
                LOG_INFO("Maybe a map from public %d to private %d?", ntohs(e->private_port),
                         ntohs(e->private_port));
            }
            LOG_WARNING("Failed to decrypt packet");
            return NULL;
        }

        return &decrypted_packet;
    } else {
        if (encrypted_len < 0) {
            int error = GetLastNetworkError();
            switch (error) {
                case FRACTAL_ETIMEDOUT:
                case FRACTAL_EWOULDBLOCK:
                    break;
                default:
                    LOG_WARNING("Unexpected Packet Error: %d", error);
                    break;
            }
        }
        return NULL;
    }
}

static int reading_packet_len;

void ClearReadingTCP(SocketContext *context) {
    UNUSED(context);
    reading_packet_len = 0;
}

FractalPacket *ReadTCPPacket(SocketContext *context) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return NULL;
    }
    if (!context->is_tcp) {
        LOG_WARNING("TryReadingTCPPacket received a context that is NOT TCP!");
        return NULL;
    }

    // NOTE: The following code only works when reading from one TCP socket.
    // Will need to be adjusted if multiple TCP sockets are used
    static char encrypted_packet_buffer[LARGEST_ENCRYPTED_TCP_PACKET];
    static char decrypted_packet_buffer[LARGEST_TCP_PACKET];

    int len;

    do {
        // Try to fill up the buffer, in chunks of TCP_SEGMENT_SIZE, but don't
        // overflow LARGEST_TCP_PACKET
        len = recvp(context, encrypted_packet_buffer + reading_packet_len,
                    min(TCP_SEGMENT_SIZE, LARGEST_ENCRYPTED_TCP_PACKET - reading_packet_len));

        if (len < 0) {
            int err = GetLastNetworkError();
            if (err == FRACTAL_ETIMEDOUT || err == FRACTAL_EAGAIN) {
            } else {
                // mprintf( "Error %d\n", err );
            }
        } else if (len > 0) {
            // mprintf( "READ LEN: %d\n", len );
            reading_packet_len += len;
        }

        // If the previous recvp was maxed out, then try pulling some more from
        // recvp
    } while (len == TCP_SEGMENT_SIZE);

    if ((unsigned long)reading_packet_len > sizeof(int)) {
        // The amount of data bytes read (actual len), and the amount of bytes
        // we're looking for (target len), respectively
        int actual_len = reading_packet_len - sizeof(int);
        int target_len = *((int *)encrypted_packet_buffer);

        // If the target len is valid, and actual len > target len, then we're
        // good to go
        if (target_len >= 0 && target_len <= LARGEST_TCP_PACKET && actual_len >= target_len) {
            // Decrypt it
            int decrypted_len =
                decrypt_packet_n((FractalPacket *)(encrypted_packet_buffer + sizeof(int)),
                                 target_len, (FractalPacket *)decrypted_packet_buffer,
                                 LARGEST_TCP_PACKET, (unsigned char *)context->aes_private_key);

            // Move the rest of the read bytes to the beginning of the buffer to
            // continue
            int start_next_bytes = sizeof(int) + target_len;
            for (unsigned long i = start_next_bytes; i < sizeof(int) + actual_len; i++) {
                encrypted_packet_buffer[i - start_next_bytes] = encrypted_packet_buffer[i];
            }
            reading_packet_len = actual_len - target_len;

            if (decrypted_len < 0) {
                LOG_WARNING("Could not decrypt TCP message");
                return NULL;
            } else {
                // Return the decrypted packet
                return (FractalPacket *)decrypted_packet_buffer;
            }
        }
    }

    return NULL;
}

int CreateTCPServerContext(SocketContext *context, int port, int recvfrom_timeout_ms,
                           int stun_timeout_ms) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    context->is_tcp = true;

    int opt;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);
    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    // Set listen queue
    set_timeout(context->s, stun_timeout_ms);
    if (listen(context->s, 3) < 0) {
        LOG_WARNING("Could not listen(2)! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    fd_set fd_read, fd_write;
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_SET(context->s, &fd_read);
    FD_SET(context->s, &fd_write);

    struct timeval tv;
    tv.tv_sec = stun_timeout_ms / MS_IN_SECOND;
    tv.tv_usec = (stun_timeout_ms % MS_IN_SECOND) * MS_IN_SECOND;

    if (select(0, &fd_read, &fd_write, NULL, stun_timeout_ms > 0 ? &tv : NULL) < 0) {
        LOG_WARNING("Could not select!");
        closesocket(context->s);
        return -1;
    }

    // Accept connection from client
    socklen_t slen = sizeof(context->addr);
    SOCKET new_socket;
    if ((new_socket = accept(context->s, (struct sockaddr *)(&context->addr), &slen)) < 0) {
        LOG_WARNING("Did not receive response from client! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    closesocket(context->s);
    context->s = new_socket;

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateTCPServerContextStun(SocketContext *context, int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    context->is_tcp = true;

    // Init stun_addr
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);
    int opt;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    SOCKET udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // cppcheck-suppress nullPointer
    sendto(udp_s, NULL, 0, 0, (struct sockaddr *)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);

    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    struct sockaddr_in origin_addr;
    // Connect over TCP to STUN
    LOG_INFO("Connecting to STUN TCP...");
    if (!tcp_connect(context->s, stun_addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to STUN Server over TCP");
        return -1;
    }

    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->s, (struct sockaddr *)&origin_addr, &slen) < 0) {
        LOG_WARNING("Could not get sock name");
        closesocket(context->s);
        return -1;
    }

    // Send STUN request
    stun_request_t stun_request;
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        LOG_WARNING("Could not send STUN request to connected STUN server!");
        closesocket(context->s);
        return -1;
    }

    // Receive STUN response
    clock t;
    StartTimer(&t);

    int recv_size = 0;
    stun_entry_t entry = {0};

    while (recv_size < (int)sizeof(entry) && GetTimer(t) < stun_timeout_ms) {
        int single_recv_size;
        if ((single_recv_size = recvp(context, ((char *)&entry) + recv_size,
                                      max(0, (int)sizeof(entry) - recv_size))) < 0) {
            LOG_WARNING("Did not receive STUN response %d\n", GetLastNetworkError());
            closesocket(context->s);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->s);
        return -1;
    }

    // Print STUN response
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = entry.ip;
    client_addr.sin_port = entry.private_port;
    LOG_INFO("TCP STUN notified of desired request from %s:%d\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));

    closesocket(context->s);

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }

    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    LOG_INFO("WAIT");

    // Connect to client
    if (!tcp_connect(context->s, client_addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to Client over TCP");
        return -1;
    }

    context->addr = client_addr;
    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));
    set_timeout(context->s, recvfrom_timeout_ms);
    return 0;
}

int CreateTCPClientContext(SocketContext *context, char *destination, int port,
                           int recvfrom_timeout_ms, int stun_timeout_ms) {
    UNUSED(stun_timeout_ms);
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    if (destination == NULL) {
        LOG_WARNING("destiniation is NULL");
        return -1;
    }
    context->is_tcp = true;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    LOG_INFO("Connecting to server...");

    SDL_Delay(200);

    // Connect to TCP server
    if (!tcp_connect(context->s, context->addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to server over TCP");
        return -1;
    }

    LOG_INFO("Connected on %s:%d!\n", destination, port);

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateTCPClientContextStun(SocketContext *context, char *destination, int port,
                               int recvfrom_timeout_ms, int stun_timeout_ms) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    if (destination == NULL) {
        LOG_WARNING("destiniation is NULL");
        return -1;
    }
    context->is_tcp = true;

    // Init stun_addr
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);
    int opt;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Tell the STUN to use TCP
    SOCKET udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    // cppcheck-suppress nullPointer
    sendto(udp_s, NULL, 0, 0, (struct sockaddr *)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);
    // Client connection protocol
    context->is_server = false;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Connect to STUN server
    if (!tcp_connect(context->s, stun_addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to STUN Server over TCP");
        return -1;
    }

    struct sockaddr_in origin_addr;
    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->s, (struct sockaddr *)&origin_addr, &slen) < 0) {
        LOG_WARNING("Could not get sock name");
        closesocket(context->s);
        return -1;
    }

    // Make STUN request
    stun_request_t stun_request;
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        LOG_WARNING("Could not send STUN request to connected STUN server!");
        closesocket(context->s);
        return -1;
    }

    // Receive STUN response
    clock t;
    StartTimer(&t);

    int recv_size = 0;
    stun_entry_t entry = {0};

    while (recv_size < (int)sizeof(entry) && GetTimer(t) < stun_timeout_ms) {
        int single_recv_size;
        if ((single_recv_size = recvp(context, ((char *)&entry) + recv_size,
                                      max(0, (int)sizeof(entry) - recv_size))) < 0) {
            LOG_WARNING("Did not receive STUN response %d\n", GetLastNetworkError());
            closesocket(context->s);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->s);
        return -1;
    }

    // Print STUN response
    struct in_addr a;
    a.s_addr = entry.ip;
    LOG_WARNING("TCP STUN responded that the TCP server is located at %s:%d\n", inet_ntoa(a),
                ntohs(entry.private_port));

    closesocket(context->s);

    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = entry.ip;
    context->addr.sin_port = entry.private_port;

    LOG_INFO("Connecting to server...");

    // Connect to TCP server
    if (!tcp_connect(context->s, context->addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to server over TCP");
        return -1;
    }

    LOG_INFO("Connected on %s:%d!\n", destination, port);

    set_timeout(context->s, recvfrom_timeout_ms);
    return 0;
}

int CreateTCPContext(SocketContext *context, char *destination, int port, int recvfrom_timeout_ms,
                     int stun_timeout_ms, bool using_stun, char *aes_private_key) {
    if (context == NULL) {
        LOG_ERROR("Context is NULL");
        return -1;
    }
    context->mutex = SDL_CreateMutex();
    memcpy(context->aes_private_key, aes_private_key, sizeof(context->aes_private_key));

    int ret;

    if (using_stun) {
        if (destination == NULL)
            ret = CreateTCPServerContextStun(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            ret = CreateTCPClientContextStun(context, destination, port, recvfrom_timeout_ms,
                                             stun_timeout_ms);
    } else {
        if (destination == NULL)
            ret = CreateTCPServerContext(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            ret = CreateTCPClientContext(context, destination, port, recvfrom_timeout_ms,
                                         stun_timeout_ms);
    }

    if (!handshakePrivateKey(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->s);
        return -1;
    }

    ClearReadingTCP(context);
    return ret;
}

int CreateUDPServerContext(SocketContext *context, int port, int recvfrom_timeout_ms,
                           int stun_timeout_ms) {
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    context->is_tcp = false;
    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout( context->s, stun_timeout_ms );
    // Server connection protocol
    context->is_server = true;

    // Bind the server port to the advertized public port
    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    if (bind(context->s, (struct sockaddr *)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    LOG_INFO("Waiting for client to connect to %s:%d...\n", "localhost", port);

    if( !handshakePrivateKey( context ) )
    {
        LOG_WARNING( "Could not complete handshake!" );
        closesocket( context->s );
        return -1;
    }

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPServerContextStun(SocketContext *context, int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    context->is_tcp = false;

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Server connection protocol
    context->is_server = true;

    // Tell the STUN to log our requested virtual port
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);

    stun_request_t stun_request;
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    LOG_INFO("Sending stun entry to STUN...");
    if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
        LOG_WARNING("Could not send message to STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    LOG_INFO("Waiting for client to connect to %s:%d...\n", "localhost", port);

    // Receive client's connection attempt
    // Update the STUN every 100ms
    set_timeout(context->s, 100);

    // But keep track of time to compare against stun_timeout_ms
    clock recv_timer;
    StartTimer(&recv_timer);

    socklen_t slen = sizeof(context->addr);
    stun_entry_t entry = {0};
    int recv_size;
    while ((recv_size = recvfrom(context->s, (char *)&entry, sizeof(entry), 0,
                                 (struct sockaddr *)(&context->addr), &slen)) < 0) {
        // If we haven't spent too much time waiting, and our previous 100ms
        // poll failed, then send another STUN update
        if (GetTimer(recv_timer) * MS_IN_SECOND < stun_timeout_ms &&
            (GetLastNetworkError() == FRACTAL_ETIMEDOUT ||
             GetLastNetworkError() == FRACTAL_EAGAIN)) {
            if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
                       (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
                LOG_WARNING("Could not send message to STUN %d\n", GetLastNetworkError());
                closesocket(context->s);
                return -1;
            }
            continue;
        }
        LOG_WARNING("Did not receive response from client! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    set_timeout(context->s, 350);

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("STUN response was not the size of an entry!");
        closesocket(context->s);
        return -1;
    }

    // Setup addr to open up port
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = entry.ip;
    context->addr.sin_port = entry.private_port;

    LOG_INFO("Received STUN response, client connection desired from %s:%d\n",
             inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));

    if (!handshakePrivateKey(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->s);
        return -1;
    }

    // Check that confirmation matches STUN's claimed client
    if (context->addr.sin_addr.s_addr != entry.ip || context->addr.sin_port != entry.private_port) {
        LOG_WARNING(
            "Connection did not match STUN's claimed client, got %s:%d "
            "instead\n",
            inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));
        context->addr.sin_addr.s_addr = entry.ip;
        context->addr.sin_port = entry.private_port;
        LOG_WARNING("Should have been %s:%d!\n", inet_ntoa(context->addr.sin_addr),
                    ntohs(context->addr.sin_port));
        closesocket(context->s);
        return -1;
    }

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPClientContext(SocketContext *context, char *destination, int port,
                           int recvfrom_timeout_ms, int stun_timeout_ms) {
    context->is_tcp = false;

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    LOG_INFO("Connecting to server...");

    if (!handshakePrivateKey(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->s);
        return -1;
    }

    LOG_WARNING("Connected to server on %s:%d! (Private %d)\n", inet_ntoa(context->addr.sin_addr),
                port, ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPClientContextStun(SocketContext *context, char *destination, int port,
                               int recvfrom_timeout_ms, int stun_timeout_ms) {
    context->is_tcp = false;

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;

    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);

    stun_request_t stun_request;
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    LOG_INFO("Sending info request to STUN...");
    if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
        LOG_WARNING("Could not send message to STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    stun_entry_t entry = {0};
    int recv_size;
    if ((recv_size = recvp(context, &entry, sizeof(entry))) < 0) {
        LOG_WARNING("Could not receive message from STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("STUN Response of incorrect size!");
        closesocket(context->s);
        return -1;
    } else if (entry.ip != stun_request.entry.ip ||
               entry.public_port != stun_request.entry.public_port) {
        LOG_WARNING("STUN Response IP and/or Public Port is incorrect!");
        closesocket(context->s);
        return -1;
    } else {
        LOG_WARNING("Received STUN response! Public %d is mapped to private %d\n",
                    ntohs((unsigned short)entry.public_port),
                    ntohs((unsigned short)entry.private_port));
        context->addr.sin_family = AF_INET;
        context->addr.sin_addr.s_addr = entry.ip;
        context->addr.sin_port = entry.private_port;
    }

    LOG_INFO("Connecting to server...");

    if( !handshakePrivateKey( context ) )
    {
        LOG_WARNING( "Could not complete handshake!" );
        closesocket( context->s );
        return -1;
    }

    LOG_INFO("Connected to server on %s:%d! (Private %d)\n", inet_ntoa(context->addr.sin_addr),
             port, ntohs(context->addr.sin_port));
    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPContext(SocketContext *context, char *destination, int port, int recvfrom_timeout_ms,
                     int stun_timeout_ms, bool using_stun, char *aes_private_key) {
    if (context == NULL) {
        LOG_ERROR("Context is NULL");
        return -1;
    }

    context->mutex = SDL_CreateMutex();
    memcpy(context->aes_private_key, aes_private_key, sizeof(context->aes_private_key));

    if (using_stun) {
        if (destination == NULL)
            return CreateUDPServerContextStun(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            return CreateUDPClientContextStun(context, destination, port, recvfrom_timeout_ms,
                                              stun_timeout_ms);
    } else {
        if (destination == NULL)
            return CreateUDPServerContext(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            return CreateUDPClientContext(context, destination, port, recvfrom_timeout_ms,
                                          stun_timeout_ms);
    }
}

// send JSON post to query the database, authenticate the user and return the VM
// IP
bool SendJSONPost(char *host_s, char *path, char *jsonObj) {
    // environment variables
    SOCKET Socket;  // socket to send/receive POST request
    struct hostent *host;
    struct sockaddr_in webserver_socketAddress;  // address of the web server socket

    // Creating our TCP socket to connect to the web server
    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket < 0)  // Windows & Unix cases
    {
        // if can't create socket, return
        LOG_WARNING("Could not create socket.");
        return false;
    }
    set_timeout(Socket, 250);

    host = gethostbyname(host_s);

    if (!host) {
        LOG_WARNING("Could not SendJSONPost to %s\n", host_s);
        return false;
    }

    // create the struct for the webserver address socket we will query
    webserver_socketAddress.sin_family = AF_INET;
    webserver_socketAddress.sin_port = htons(80);  // HTTP port
    webserver_socketAddress.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);

    // connect to the web server before sending the POST request packet
    int connect_status = connect(Socket, (struct sockaddr *)&webserver_socketAddress,
                                 sizeof(webserver_socketAddress));
    if (connect_status < 0) {
        LOG_WARNING("Could not connect to the webserver.");
        return false;
    }

    // now that we're connected, we can send the POST request to authenticate
    // the user first, we create the POST request message

    int json_len = (int)strlen(jsonObj);
    char *message = malloc(5000 + json_len);
    snprintf(message, 5000 + json_len,
             "POST %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s\r\n",
             path, host_s, json_len, jsonObj);

    // now we send it
    if (send(Socket, message, (int)strlen(message), 0) < 0) {
        // error sending, terminate
        LOG_WARNING("Sending POST message failed.");
        free(message);
        return false;
    }

    free(message);

    // now that it's sent, let's get the reply
    char buffer[4096];                              // buffer to store the reply
    int len;                                        // counters
    len = recv(Socket, buffer, sizeof(buffer), 0);  // get the reply

    // get the parsed credentials
    for (int i = 0; i < len; i++) {
        if (buffer[i] == '\r') {
            buffer[i] = '\0';
        }
    }
    LOG_INFO("POST Request Webserver Response: %s\n", buffer);

    FRACTAL_CLOSE_SOCKET(Socket);
    // return the user credentials if correct authentication, else empty
    return true;
}

// send JSON get to query the database for VM details
bool SendJSONGet(char *host_s, char *path, char *json_res, size_t json_res_size) {
    // environment variables
    SOCKET Socket;  // socket to send/receive POST request
    struct hostent *host;
    struct sockaddr_in webserver_socketAddress;  // address of the web server socket

    // Creating our TCP socket to connect to the web server
    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket < 0)  // Windows & Unix cases
    {
        // if can't create socket, return
        LOG_WARNING("Could not create socket.");
        return false;
    }
    set_timeout(Socket, 250);

    host = gethostbyname(host_s);

    // create the struct for the webserver address socket we will query
    webserver_socketAddress.sin_family = AF_INET;
    webserver_socketAddress.sin_port = htons(80);  // HTTP port
    webserver_socketAddress.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);

    // connect to the web server before sending the POST request packet
    int connect_status = connect(Socket, (struct sockaddr *)&webserver_socketAddress,
                                 sizeof(webserver_socketAddress));
    if (connect_status < 0) {
        LOG_WARNING("Could not connect to the webserver.");
        return false;
    }

    // now that we're connected, we can send the POST request to authenticate
    // the user first, we create the POST request message
    char *message = malloc(250);
    sprintf(message, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host_s);

    // now we send it
    if (send(Socket, message, (int)strlen(message), 0) < 0) {
        // error sending, terminate
        LOG_WARNING("Sending GET message failed.");
        free(message);
        return false;
    }

    free(message);

    // now that it's sent, let's get the reply
    int len = recv(Socket, json_res, (int)json_res_size - 1, 0);  // get the reply
    if (len < 0) {
        LOG_WARNING("Response to JSON GET failed!");
        json_res[0] = '\0';
    } else {
        json_res[len] = '\0';
        LOG_INFO("JSON GET Response: %s", json_res);
    }

    FRACTAL_CLOSE_SOCKET(Socket);
    return true;
}

void set_timeout(SOCKET s, int timeout_ms) {
    // Sets the timeout for SOCKET s to be timeout_ms in milliseconds
    // Any recv calls will wait this long before timing out
    // -1 means that it will block indefinitely until a packet is received
    // 0 means that it will immediately return with whatever data is waiting in
    // the buffer
    if (timeout_ms < 0) {
        LOG_WARNING(
            "WARNING: This socket will blocking indefinitely. You will not be "
            "able to recover if a packet is never received");
        unsigned long mode = 0;

        FRACTAL_IOCTL_SOCKET(s, FIONBIO, &mode);

    } else if (timeout_ms == 0) {
        unsigned long mode = 1;
        FRACTAL_IOCTL_SOCKET(s, FIONBIO, &mode);
    } else {
        // Set to blocking when setting a timeout
        unsigned long mode = 0;
        FRACTAL_IOCTL_SOCKET(s, FIONBIO, &mode);

        clock read_timeout = CreateClock(timeout_ms);

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&read_timeout,
                       sizeof(read_timeout)) < 0) {
            LOG_WARNING("Failed to set timeout");
            return;
        }
    }
}

void preparePrivateKeyRequest(private_key_data_t *priv_key_data) {
    gen_iv(priv_key_data->iv);
}

typedef struct {
    char iv[16];
    char private_key[16];
} signature_data_t;

bool signPrivateKey(private_key_data_t *priv_key_data, int recv_size, char *private_key) {
    if (recv_size == sizeof(private_key_data_t)) {
        signature_data_t sig_data;
        memcpy(sig_data.iv, priv_key_data->iv, sizeof(priv_key_data->iv));
        memcpy(sig_data.private_key, private_key, sizeof(sig_data.private_key));
        hmac(priv_key_data->signature, &sig_data, sizeof(sig_data), private_key);
        return true;
    } else {
        LOG_ERROR("Recv Size was not equal to private_key_data_t: %d instead of %d", recv_size,
                  sizeof(private_key_data_t));
        return false;
    }
}

bool confirmPrivateKey(private_key_data_t *our_priv_key_data,
                       private_key_data_t *our_signed_priv_key_data, int recv_size,
                       char *private_key) {
    if (recv_size == sizeof(private_key_data_t)) {
        if (memcmp(our_priv_key_data->iv, our_signed_priv_key_data->iv, 16) != 0) {
            LOG_ERROR("IV is incorrect!");
            return false;
        } else {
            signature_data_t sig_data;
            memcpy(sig_data.iv, our_signed_priv_key_data->iv, sizeof(our_signed_priv_key_data->iv));
            memcpy(sig_data.private_key, private_key, sizeof(sig_data.private_key));
            if (!verify_hmac(our_signed_priv_key_data->signature, &sig_data, sizeof(sig_data),
                             private_key)) {
                LOG_ERROR("Verify HMAC Failed");
                return false;
            } else {
                return true;
            }
        }
    } else {
        LOG_ERROR("Recv Size was not equal to private_key_data_t: %d instead of %d", recv_size,
                  sizeof(private_key_data_t));
        return false;
    }
}
