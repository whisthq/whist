/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network.c
 * @brief This file contains all code that interacts directly with sockets
 *        under-the-hood.
 */

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, needs to
                                         // be at the very top
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <whist/core/whist.h>

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
Public Interface Implementations
============================
*/

int ack(SocketContext* context) {
    if (context->context == NULL) {
        LOG_ERROR("The given SocketContext has not been initialized!");
        return -1;
    }
    return context->ack(context->context);
}

WhistPacket* read_packet(SocketContext* context, bool should_recv) {
    if (context->context == NULL) {
        LOG_ERROR("The given SocketContext has not been initialized!");
        return NULL;
    }
    return context->read_packet(context->context, should_recv);
}

void free_packet(SocketContext* context, WhistPacket* packet) {
    if (context->context == NULL) {
        LOG_ERROR("The given SocketContext has not been initialized!");
        return;
    }
    context->free_packet(context->context, packet);
}

int send_packet(SocketContext* context, WhistPacketType packet_type, void* payload,
                int payload_size, int packet_id) {
    if (context->context == NULL) {
        LOG_ERROR("The given SocketContext has not been initialized!");
        return -1;
    }
    return context->send_packet(context->context, packet_type, payload, payload_size, packet_id);
}

void destroy_socket_context(SocketContext* context) {
    if (context->context == NULL) {
        LOG_ERROR("The given SocketContext has not been initialized!");
        return;
    }
    context->destroy_socket_context(context->context);
    memset(context, 0, sizeof(*context));
}

/*
============================
Private Function Declarations
============================
*/

typedef struct {
    char iv[16];
    char signature[32];
} PrivateKeyData;

typedef struct {
    char iv[16];
    char private_key[16];
} SignatureData;

/**
 * @brief                          This will prepare the private key data
 *
 * @param priv_key_data            The private key data buffer
 */
void prepare_private_key_request(PrivateKeyData* priv_key_data);

/**
 * @brief                          This will sign the other connection's private key data
 *
 * @param priv_key_data            The private key data buffer
 * @param recv_size                The length of the buffer
 * @param private_key              The private key
 *
 * @returns                        True if the verification succeeds, false if it fails
 */
bool sign_private_key(PrivateKeyData* priv_key_data, int recv_size, void* private_key);

/**
 * @brief                          This will verify the given private key
 *
 * @param our_priv_key_data        The private key data buffer
 * @param our_signed_priv_key_data The signed private key data buffer
 * @param recv_size                The length of the buffer
 * @param private_key              The private key
 *
 * @returns                        True if the verification succeeds, false if it fails
 */
bool confirm_private_key(PrivateKeyData* our_priv_key_data,
                         PrivateKeyData* our_signed_priv_key_data, int recv_size,
                         void* private_key);

/*
============================
Public Function Implementations
============================
*/

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

        if (WHIST_IOCTL_SOCKET(socket, FIONBIO, &mode) != 0) {
            LOG_FATAL("Failed to make socket blocking.");
        }

    } else if (timeout_ms == 0) {
        unsigned long mode = 1;
        if (WHIST_IOCTL_SOCKET(socket, FIONBIO, &mode) != 0) {
            LOG_FATAL("Failed to make socket return immediately.");
        }
    } else {
        // Set to blocking when setting a timeout
        unsigned long mode = 0;
        if (WHIST_IOCTL_SOCKET(socket, FIONBIO, &mode) != 0) {
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

void set_tos(SOCKET socket, WhistTOSValue tos) {
    /*
        Sets the TOS value for `socket` to be `tos`. This is used to set the type of
        service header in the packet, which is used by routers to determine routing/
        queueing/dropping behavior.

        Arguments:
            socket (SOCKET): the socket to set TOS for
            tos (WhistTOSValue): the TOS value to set. This should correspond
                 to a DSCP value (see https://www.tucny.com/Home/dscp-tos)
    */
    int opt = tos;
    if (setsockopt(socket, IPPROTO_IP, IP_TOS, (const char*)&opt, sizeof(opt)) < 0) {
        int err = get_last_network_error();
        LOG_WARNING("Failed to set TOS to %d: %d. Msg: %s\n", opt, err, strerror(err));
    }
}

void whist_init_networking() {
    /*
        Initialize default port mappings (i.e. the identity)
    */

    for (int i = 0; i <= USHRT_MAX; i++) {
        port_mappings[i] = (unsigned short)i;
    }

    // initialize the windows socket library if this is on windows
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        LOG_FATAL("Failed to initialize Winsock with error code: %d.", WSAGetLastError());
    }
    // We don't call WSACleanup(), but it's not a big deal
#endif
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

int get_packet_size(WhistPacket* packet) {
    /*
        Get the size of a WhistPacket

        Arguments:
            packet (WhistPacket*): The packet to get the size of.

        Returns:
            (int): The size of the packet, or -1 on error.
    */

    if (packet == NULL) {
        LOG_ERROR("Packet is NULL");
        return -1;
    }

    return PACKET_HEADER_SIZE + packet->payload_size;
}

bool handshake_private_key(SocketContextData* context) {
    /*
        Perform a private key handshake with a peer.

        Arguments:
            context (SocketContextData*): the socket context to be used

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
    if (send(context->socket, (const char*)&our_priv_key_data, sizeof(our_priv_key_data), 0) < 0) {
        LOG_ERROR("send(3) failed! Could not send private key request data! %d",
                  get_last_network_error());
        return false;
    }

    // Receive, sign, and send back their private key request data
    int cnt = 0;
    while ((recv_size =
                recvfrom(context->socket, (char*)&their_priv_key_data, sizeof(their_priv_key_data),
                         0, (struct sockaddr*)(&context->addr), &slen)) == 0) {
        if (cnt >= 3)  // we are (very likely) getting a dead loop casued by stream socket closed
        {              // the loop should be okay to be removed, just kept for debugging.
            return false;
        }
        cnt++;
        LOG_INFO("recv_size()==0, cnt=%d", cnt);
    }

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
    if (send(context->socket, (const char*)&their_priv_key_data, sizeof(their_priv_key_data), 0) <
        0) {
        LOG_ERROR("send(3) failed! Could not send signed private key data! %d",
                  get_last_network_error());
        return false;
    }

    // Wait for and verify their signed private key request data
    recv_size = recv(context->socket, (char*)&our_signed_priv_key_data,
                     sizeof(our_signed_priv_key_data), 0);
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

/*
============================
Private Function Implementations
============================
*/

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
