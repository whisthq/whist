/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file network.c
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

#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, need to
                                         // be at the very top
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "network.h"

#include <stdio.h>
#include <fcntl.h>

#ifndef _WIN32
#include "curl/curl.h"
#else
#include <winhttp.h>
#endif

#include "../utils/aes.h"

// Defines

#define BITS_IN_BYTE 8.0
#define MS_IN_SECOND 1000

// Global data
unsigned short port_mappings[USHRT_MAX + 1];

/*
============================
Private Custom Types
============================
*/

/**
 * @brief                       Struct to keep response data from a curl
 *                              request as it is being received.
 */
typedef struct {
    char* buffer;       // Buffer that contains received response data
    size_t filled_len;  // How much of the buffer is full so far
    size_t max_len;     // How much the buffer can be filled, at most
} CurlResponseBuffer;

/*
============================
Private Functions
============================
*/

/*
============================
Public Function Implementations
============================
*/

FractalPacket* read_packet(SocketContext* context, bool should_recvp) {
    return context->read_packet(context->context, should_recvp);
}
int send_packet_from_payload(SocketContext* context, FractalPacketType type, void* data, int len,
                             int id) {
    return context->send_packet_from_payload(context->context, type, data, len, id);
}
int send_packet(SocketContext* context, FractalPacket* packet, size_t packet_size) {
    return context->send_packet(context->context, packet, packet_size);
}
void free_packet(SocketContext* context, FractalPacket* packet) { context->free_packet(packet); }
int write_payload_to_packets(SocketContext* context, uint8_t* payload, size_t payload_size,
                             int payload_id, FractalPacketType packet_type,
                             FractalPacket* packet_buffer, size_t packet_buffer_length) {
    return context->write_payload_to_packets(payload, payload_size, payload_id, packet_type,
                                             packet_buffer, packet_buffer_length);
}
void destroy_socket_context(SocketContext* context) {
    closesocket(((SocketAttributes*)context->context)->socket);
    NetworkThrottleContext* network_throttler =
        ((SocketAttributes*)context->context)->network_throttler;
    if (!((SocketAttributes*)context->context)->is_tcp && network_throttler != NULL) {
        network_throttler_destroy(network_throttler);
    }
    free(context->context);
}

/*
============================
Private Function Implementations
============================
*/

SOCKET socketp_tcp() {
    /*
        Create a TCP socket and set the FD_CLOEXEC flag.
        Linux permits atomic FD_CLOEXEC definition via SOCK_CLOEXEC,
        but this is not available on other operating systems yet.

        Return:
            SOCKET: socket fd on success; INVALID_SOCKET on failure
    */

#ifdef SOCK_CLOEXEC
    // Create socket
    SOCKET sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sock_fd <= 0) {
        LOG_WARNING("Could not create socket %d\n", get_last_network_error());
        return INVALID_SOCKET;
    }
#else
    // Create socket
    SOCKET sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create socket %d\n", get_last_network_error());
        return INVALID_SOCKET;
    }

#ifndef _WIN32
    // Set socket to close on child exec
    // Not necessary for windows because CreateProcessA creates an independent process
    if (fcntl(sock_fd, F_SETFD, fcntl(sock_fd, F_GETFD) | FD_CLOEXEC) < 0) {
        LOG_WARNING("Could not set fcntl to set socket to close on child exec");
        return INVALID_SOCKET;
    }
#endif
#endif

    return sock_fd;
}

SOCKET socketp_udp() {
    /*
        Create a UDP socket and set the FD_CLOEXEC flag.
        Linux permits atomic FD_CLOEXEC definition via SOCK_CLOEXEC,
        but this is not available on other operating systems yet.

        Return:
            SOCKET: socket fd on success; INVALID_SOCKET on failure
    */

#ifdef SOCK_CLOEXEC
    // Create socket
    SOCKET sock_fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
#else
    // Create socket
    SOCKET sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

#ifndef _WIN32
    // Set socket to close on child exec
    // Not necessary for windows because CreateProcessA creates an independent process
    if (fcntl(sock_fd, F_SETFD, fcntl(sock_fd, F_GETFD) | FD_CLOEXEC) < 0) {
        LOG_WARNING("Could not set fcntl to set socket to close on child exec");
        return INVALID_SOCKET;
    }
#endif
#endif

    return sock_fd;
}

bool handshake_private_key(SocketAttributes* context) {
    /*
        Perform a private key handshake with a peer.

        Arguments:
            context (SocketAttributes*): the socket context to be used

        Returns:
            (bool): True on success, False on failure
    */

    set_timeout(context->socket, 1000);

    PrivateKeyData our_priv_key_data;
    PrivateKeyData our_signed_priv_key_data;
    PrivateKeyData their_priv_key_data;
    int recv_size;
    socklen_t slen = sizeof(context->addr);

    // Generate and send private key request data
    prepare_private_key_request(&our_priv_key_data);
    if (sendp(context, &our_priv_key_data, sizeof(our_priv_key_data)) < 0) {
        LOG_ERROR("sendp(3) failed! Could not send private key request data! %d",
                  get_last_network_error());
        return false;
    }
    fractal_sleep(50);

    // Receive, sign, and send back their private key request data
    while ((recv_size =
                recvfrom(context->socket, (char*)&their_priv_key_data, sizeof(their_priv_key_data),
                         0, (struct sockaddr*)(&context->addr), &slen)) == 0)
        ;
    if (recv_size < 0) {
        LOG_WARNING("Did not receive other connection's private key request: %d",
                    get_last_network_error());
        return false;
    }
    LOG_INFO("Private key request received");
    if (!sign_private_key(&their_priv_key_data, recv_size, context->binary_aes_private_key)) {
        LOG_ERROR("signPrivateKey failed!");
        return false;
    }
    if (sendp(context, &their_priv_key_data, sizeof(their_priv_key_data)) < 0) {
        LOG_ERROR("sendp(3) failed! Could not send signed private key data! %d",
                  get_last_network_error());
        return false;
    }
    fractal_sleep(50);

    // Wait for and verify their signed private key request data
    recv_size = recvp(context, &our_signed_priv_key_data, sizeof(our_signed_priv_key_data));
    if (!confirm_private_key(&our_priv_key_data, &our_signed_priv_key_data, recv_size,
                             context->binary_aes_private_key)) {
        LOG_ERROR("Could not confirmPrivateKey!");
        return false;
    } else {
        LOG_INFO("Private key confirmed");
        set_timeout(context->socket, context->timeout);
        return true;
    }
}

size_t write_curl_response_callback(char* ptr, size_t size, size_t nmemb, CurlResponseBuffer* crb) {
    /*
    Writes CURL request response data to the CurlResponseBuffer buffer up to `crb->max_len`

    Arguments:
        ptr (char*): pointer to the delivered response data
        size (size_t): always 1 - size of each `nmemb` pointed to by `ptr`
        nmemb (size_t): the size of the buffer pointed to by `ptr`
        crb (CurlResponseBuffer*): the response buffer object that contains full response data

    Returns:
        size (size_t): returns `size * nmemb` if successful (this function just always
            returns success state)
    */

    if (!crb || !crb->buffer || crb->filled_len > crb->max_len) {
        return size * nmemb;
    }

    size_t copy_bytes = size * nmemb;
    if (copy_bytes + crb->filled_len > crb->max_len) {
        copy_bytes = crb->max_len - crb->filled_len;
    }

    memcpy(crb->buffer + crb->filled_len, ptr, copy_bytes);

    return size * nmemb;
}

void set_timeout(SOCKET socket, int timeout_ms) {
    /*
        Sets the timeout for `socket` to be `timeout_ms` in milliseconds.
        Any recv calls will wait this long before timing out.

        Arguments:
            socket (SOCKET): the socket to set timeout for
            timeout_ms (int): a positibe number is the timeout in milliseconds.
                -1 means that it will block indefinitely until a packet is
                received. 0 means that it will immediately return with whatever
                data is waiting in the buffer.
    */

    if (timeout_ms < 0) {
        LOG_WARNING(
            "WARNING: This socket will blocking indefinitely. You will not be "
            "able to recover if a packet is never received");
        unsigned long mode = 0;

        if (FRACTAL_IOCTL_SOCKET(socket, FIONBIO, &mode) != 0) {
            LOG_FATAL("Failed to make socket blocking.");
        }

    } else if (timeout_ms == 0) {
        unsigned long mode = 1;
        if (FRACTAL_IOCTL_SOCKET(socket, FIONBIO, &mode) != 0) {
            LOG_FATAL("Failed to make socket return immediately.");
        }
    } else {
        // Set to blocking when setting a timeout
        unsigned long mode = 0;
        if (FRACTAL_IOCTL_SOCKET(socket, FIONBIO, &mode) != 0) {
            LOG_FATAL("Failed to make socket blocking.");
        }

        clock read_timeout = create_clock(timeout_ms);

        if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&read_timeout,
                       sizeof(read_timeout)) < 0) {
            int err = get_last_network_error();
            LOG_WARNING("Failed to set timeout: %d. Msg: %s\n", err, strerror(err));

            return;
        }
    }
}

void prepare_private_key_request(PrivateKeyData* priv_key_data) {
    /*
        This will prepare the private key data

        Arguments:
            priv_key_data (PrivateKeyData*): The private key data buffer
    */

    // Generate the IV, so that someone else can sign it
    gen_iv(priv_key_data->iv);
    // Clear priv_key_data so that PrivateKeyData is entirely initialized
    // (For valgrind)
    memset(priv_key_data->signature, 0, sizeof(priv_key_data->signature));
}

bool sign_private_key(PrivateKeyData* priv_key_data, int recv_size, void* private_key) {
    /*
        This will sign the other connection's private key data

        Arguments:
            priv_key_data (PrivateKeyData*): The private key data buffer
            recv_size (int): The length of the buffer
            private_key (void*): The private key

        Returns:
            (bool): True if the verification succeeds, false if it fails
    */

    if (recv_size == sizeof(PrivateKeyData)) {
        SignatureData sig_data;
        memcpy(sig_data.iv, priv_key_data->iv, sizeof(priv_key_data->iv));
        memcpy(sig_data.private_key, private_key, sizeof(sig_data.private_key));
        hmac(priv_key_data->signature, &sig_data, sizeof(sig_data), private_key);
        return true;
    } else {
        LOG_ERROR("Recv Size was not equal to PrivateKeyData: %d instead of %d", recv_size,
                  sizeof(PrivateKeyData));
        return false;
    }
}

bool confirm_private_key(PrivateKeyData* our_priv_key_data,
                         PrivateKeyData* our_signed_priv_key_data, int recv_size,
                         void* private_key) {
    /*
        This will verify the given private key

        Arguments:
            our_priv_key_data (PrivateKeyData*): The private key data buffer
            our_signed_priv_key_data (PrivateKeyData*): The signed private key data buffer
            recv_size (int): The length of the buffer
            private_key (void*): The private key

        Returns:
            (bool): True if the verification succeeds, false if it fails
    */

    if (recv_size == sizeof(PrivateKeyData)) {
        if (memcmp(our_priv_key_data->iv, our_signed_priv_key_data->iv, 16) != 0) {
            LOG_ERROR("IV is incorrect!");
            return false;
        } else {
            SignatureData sig_data;
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
        LOG_ERROR("Recv Size was not equal to PrivateKeyData: %d instead of %d", recv_size,
                  sizeof(PrivateKeyData));
        return false;
    }
}

/*
============================
Public Function Implementations
============================
*/

void init_networking() {
    /*
        Initialize default port mappings (i.e. the identity)
    */

    for (int i = 0; i <= USHRT_MAX; i++) {
        port_mappings[i] = (unsigned short)i;
    }
}

int get_last_network_error() {
    /*
        Get the most recent network error.

        Returns:
            (int): The network error that most recently occured,
                through WSAGetLastError on Windows or errno on Linux
    */

#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

int recvp(SocketAttributes* context, void* buf, int len) {
    /*
        This will receive data over a socket

        Arguments:
            context (SocketAttributes*): The socket context to be used
            buf (void*): The buffer to write to
            len (int): The maximum number of bytes that can be read
                from the socket

        Returns:
            (int): The number of bytes that have been read into the
                buffer
    */

    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    return recv(context->socket, buf, len, 0);
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int sendp(SocketAttributes* context, void* buf, int len) {
    /*
        This will send data over a socket

        Arguments:
            context (SocketAttributes*): The socket context to be used
            buf (void*): The buffer to read from
            len (int): The length of the buffer to send over the socket

        Returns:
            (int): The number of bytes that have been read from the buffer
    */

    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    if (len != 0 && buf == NULL) {
        LOG_ERROR("Passed non zero length and a NULL pointer to sendto");
        return -1;
    }
    // cppcheck-suppress nullPointer
    if (context->is_tcp) {
        return send(context->socket, buf, len, 0);
    } else {
        if (!context->udp_is_connected) {
            // Connect to the remote address. This means that all subsequent `sendto`
            // calls can skip this overhead, since the address is cached.
            int ret =
                connect(context->socket, (struct sockaddr*)&context->addr, sizeof(context->addr));
            if (ret == -1) {
                LOG_ERROR("Failed to UDP connect to remote address: %s", strerror(errno));
                return -1;
            }
            context->udp_is_connected = true;
        }
        return sendto(context->socket, buf, len, 0, (struct sockaddr*)(NULL),
                      sizeof(context->addr));
    }
}

int get_packet_size(FractalPacket* packet) {
    /*
        Get the size of a FractalPacket

        Arguments:
            packet (FractalPacket*): The packet to get the size of.

        Returns:
            (int): The size of the packet, or -1 on error.
    */

    if (packet == NULL) {
        LOG_ERROR("Packet is NULL");
        return -1;
    }

    return PACKET_HEADER_SIZE + packet->payload_size;
}

int ack(SocketContext* context) { return context->ack(context->context); }
