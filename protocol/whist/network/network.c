/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network.c
 * @brief This file contains all code that interacts directly with sockets
 *        under-the-hood.
 */

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, needs to
                                         // be at the very top
#endif

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>

#include <stdio.h>
#include <fcntl.h>

#ifndef _WIN32
#include <sys/poll.h>
#include "curl/curl.h"
#else
#include <winhttp.h>
#endif

#include "../utils/aes.h"

/*
============================
Defines
============================
*/

#define BITS_IN_BYTE 8.0
#define MS_IN_SECOND 1000

/*
============================
Globals
============================
*/

unsigned short port_mappings[USHRT_MAX + 1];

/*
============================
Public Function Implementations
============================
*/

bool socket_update(SocketContext* context) {
    FATAL_ASSERT(context != NULL);
    return context->socket_update(context->context);
}

int send_packet(SocketContext* context, WhistPacketType packet_type, void* payload,
                int payload_size, int packet_id, bool start_of_stream) {
    FATAL_ASSERT(context != NULL);
    return context->send_packet(context->context, packet_type, payload, payload_size, packet_id,
                                start_of_stream);
}

void* get_packet(SocketContext* context, WhistPacketType type) {
    FATAL_ASSERT(context != NULL);
    return context->get_packet(context->context, type);
}

void free_packet(SocketContext* context, WhistPacket* packet) {
    FATAL_ASSERT(context != NULL);
    context->free_packet(context->context, packet);
}

bool get_pending_stream_reset(SocketContext* context, WhistPacketType type) {
    FATAL_ASSERT(context != NULL);
    return context->get_pending_stream_reset(context->context, type);
}

void destroy_socket_context(SocketContext* context) {
    FATAL_ASSERT(context != NULL);
    context->destroy_socket_context(context->context);
    memset(context, 0, sizeof(*context));
}

/*
============================
Private Function Declarations
============================
*/

// This is what gets transmitted,
// where "signature"
typedef struct {
    char iv[IV_SIZE];
    char signature[HMAC_SIZE];
} PrivateKeyData;

// This is what gets signed
typedef struct {
    char iv[IV_SIZE];
    char private_key[KEY_SIZE];
} SignatureData;

/**
 * @brief                          This will prepare the private key data
 *
 * @param priv_key_data            The private key data buffer
 */
static void prepare_private_key_request(PrivateKeyData* priv_key_data);

/**
 * @brief                          This will sign the other connection's private key data
 *
 * @param priv_key_data            The private key data buffer
 * @param recv_size                The length of the buffer
 * @param private_key              The private key
 *
 * @returns                        True if the verification succeeds, false if it fails
 */
static bool sign_private_key(PrivateKeyData* priv_key_data, int recv_size, void* private_key);

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
static bool confirm_private_key(PrivateKeyData* our_priv_key_data,
                                PrivateKeyData* our_signed_priv_key_data, int recv_size,
                                void* private_key);

/*
============================
Public Function Implementations
============================
*/

SOCKET socketp_udp(void) {
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

        // setsockopt(SO_RCVTIMEO) argument is struct timeval on Unices
        // but DWORD in milliseconds on Windows.
#ifdef _WIN32
        DWORD read_timeout = timeout_ms;
#else
        struct timeval read_timeout = {.tv_sec = timeout_ms / MS_IN_SECOND,
                                       .tv_usec = timeout_ms % MS_IN_SECOND * US_IN_MS};
#endif

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

void whist_init_networking(void) {
    // Initialize any uninitialized port mappings with the identity
    for (int i = 0; i <= USHRT_MAX; i++) {
        if (port_mappings[i] == 0) {
            port_mappings[i] = (unsigned short)i;
        }
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

int get_last_network_error(void) {
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

bool handshake_private_key(SOCKET socket, int connection_timeout_ms, const void* private_key) {
    /*
        Perform a private key handshake with a peer.

        Arguments:
            context (SocketContextData*): the socket context to be used

        Returns:
            (bool): True on success, False on failure
    */

    // Set the timeout
    set_timeout(socket, connection_timeout_ms);

    PrivateKeyData our_priv_key_data;
    PrivateKeyData our_signed_priv_key_data;
    PrivateKeyData their_priv_key_data;
    int recv_size;
    socklen_t slen;
    struct sockaddr received_addr;
    // TODO: We ignore received_addr,
    // So the addr that signed the private key, might not be
    // The addr that originally tried to connect
    // #4785 for the fix on this issue
    // This is not a private issue, since we still encrypt the transmission
    // and presume all received packets are malicious until successfully decrypted

    // Generate and send private key request data
    prepare_private_key_request(&our_priv_key_data);
    if (send(socket, (const char*)&our_priv_key_data, sizeof(our_priv_key_data), 0) < 0) {
        LOG_ERROR("send(3) failed! Could not send private key request data! %d",
                  get_last_network_error());
        return false;
    }

    // Receive, sign, and send back their private key request data
    int cnt = 0;
    slen = sizeof(received_addr);
    while ((recv_size = recvfrom_no_intr(socket, (char*)&their_priv_key_data,
                                         sizeof(their_priv_key_data), 0, &received_addr, &slen)) ==
           0) {
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
    if (!sign_private_key(&their_priv_key_data, recv_size, (void*)private_key)) {
        LOG_ERROR("signPrivateKey failed!");
        return false;
    }
    if (send(socket, (const char*)&their_priv_key_data, sizeof(their_priv_key_data), 0) < 0) {
        LOG_ERROR("send(3) failed! Could not send signed private key data! %d",
                  get_last_network_error());
        return false;
    }

    // Wait for and verify their signed private key request data
    slen = sizeof(received_addr);
    recv_size = recvfrom_no_intr(socket, (char*)&our_signed_priv_key_data,
                                 sizeof(our_signed_priv_key_data), 0, &received_addr, &slen);
    if (recv_size < 0) {
        LOG_WARNING("Did not receive our signed private key request: %d", get_last_network_error());
        return false;
    }
    if (!confirm_private_key(&our_priv_key_data, &our_signed_priv_key_data, recv_size,
                             (void*)private_key)) {
        // we LOG_ERROR and its context within the confirm_private_key function
        return false;
    } else {
        return true;
    }
}

#ifndef _WIN32
// Receive implementations avoiding EINTR.

// Note that this get_timeout() implementation will not work on Windows
// (because the non-blocking state of a socket cannot be queried), hence
// it is not made public.
static int get_timeout(SOCKET socket) {
    int ret, err;

    ret = fcntl(socket, F_GETFL);
    if (ret < 0) {
        err = get_last_network_error();
        LOG_WARNING("Failed to retrieve socket non-blocking setting: %d, %s.\n", err,
                    strerror(err));
    } else if (ret & O_NONBLOCK) {
        // Socket is non-blocking, so the timeout is zero.
        return 0;
    }

    struct timeval read_timeout;
    socklen_t opt_len = sizeof(read_timeout);
    ret = getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, &opt_len);
    if (ret < 0) {
        err = get_last_network_error();
        LOG_WARNING("Failed to retrieve socket receive timeout: %d, %s.\n", err, strerror(err));
        return -1;
    }
    if (read_timeout.tv_sec == 0 && read_timeout.tv_usec == 0)
        return -1;
    else
        return read_timeout.tv_sec * MS_IN_SECOND + read_timeout.tv_usec / US_IN_MS;
}
#endif

int recv_no_intr(SOCKET sockfd, void* buf, size_t len, int flags) {
#ifdef _WIN32
    // EINTR doesn't happen on windows, so just use the system call
    return recv(sockfd, buf, (int)len, flags);
#else
    ssize_t ret;
    bool got_timeout = false;
    int original_timeout, current_timeout;
    WhistTimer timer;
    // For timeouts we only care about how long has elapsed since this call started.
    start_timer(&timer);
    while (1) {
        ret = recv(sockfd, buf, len, flags);
        if (ret >= 0 || errno != EINTR) {
            // If we didn't hit an EINTR case, return immediately.
            return ret;
        }
        if (!got_timeout) {
            // Only fetch the timeout the first time around.
            current_timeout = original_timeout = get_timeout(sockfd);
            got_timeout = true;
        }
        // cppcheck-suppress uninitvar
        if (current_timeout <= 0) {
            // If either the socket is non-blocking (0) or there wasn't a
            // timeout set (-1) then we just call again.
            continue;
        }
        while (1) {
            // If there was a timeout set we need compare against how long
            // has actually elapsed.
            int elapsed = (int)(get_timer(&timer) * MS_IN_SECOND);
            if (elapsed >= original_timeout) {
                // If the full time has already elapsed we should return.
                // Set errno to the expected value for a timeout case.
                errno = EAGAIN;
                return -1;
            }
            // Now wait for the remaining timeout for anything to happen.
            current_timeout = original_timeout - elapsed;
            struct pollfd pfd = {
                .fd = sockfd,
                .events = POLLIN,
            };
            ret = poll(&pfd, 1, current_timeout);
            if (ret == 0) {
                // We timed out, so return that.
                errno = EAGAIN;
                return -1;
            }
            if (ret >= 0 || errno != EINTR) {
                // Either something is now there so we can recv() it, or
                // it is an external error and we want to return whatever
                // recv() says the error is.
                break;
            }
            // We got EINTR again in poll(), so recalculate the timeout
            // and go around again.
        }
    }
#endif
}

// This is identical to the previous function except for the recv() call.
// Any changes should be kept in sync between them.
int recvfrom_no_intr(SOCKET sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr,
                     socklen_t* addrlen) {
#ifdef _WIN32
    // EINTR doesn't happen on windows, so just use the system call
    return recvfrom(sockfd, buf, (int)len, flags, src_addr, addrlen);
#else
    ssize_t ret;
    bool got_timeout = false;
    int original_timeout, current_timeout;
    WhistTimer timer;
    // For timeouts we only care about how long has elapsed since this call started.
    start_timer(&timer);
    while (1) {
        ret = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
        if (ret >= 0 || errno != EINTR) {
            // If we didn't hit an EINTR case, return immediately.
            return ret;
        }
        if (!got_timeout) {
            // Only fetch the timeout the first time around.
            current_timeout = original_timeout = get_timeout(sockfd);
            got_timeout = true;
        }
        // cppcheck-suppress uninitvar
        if (current_timeout <= 0) {
            // If either the socket is non-blocking (0) or there wasn't a
            // timeout set (-1) then we just call again.
            continue;
        }
        while (1) {
            // If there was a timeout set we need compare against how long
            // has actually elapsed.
            int elapsed = (int)(get_timer(&timer) * MS_IN_SECOND);
            if (elapsed >= original_timeout) {
                // If the full time has already elapsed we should return.
                // Set errno to the expected value for a timeout case.
                errno = EAGAIN;
                return -1;
            }
            // Now wait for the remaining timeout for anything to happen.
            current_timeout = original_timeout - elapsed;
            struct pollfd pfd = {
                .fd = sockfd,
                .events = POLLIN,
            };
            ret = poll(&pfd, 1, current_timeout);
            if (ret == 0) {
                // We timed out, so return that.
                errno = EAGAIN;
                return -1;
            }
            if (ret >= 0 || errno != EINTR) {
                // Either something is now there so we can recv() it, or
                // it is an external error and we want to return whatever
                // recv() says the error is.
                break;
            }
            // We got EINTR again in poll(), so recalculate the timeout
            // and go around again.
        }
    }
#endif
}

/*
============================
Private Function Implementations
============================
*/

static void prepare_private_key_request(PrivateKeyData* priv_key_data) {
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

static bool sign_private_key(PrivateKeyData* priv_key_data, int recv_size, void* private_key) {
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
        LOG_ERROR("Recv Size was not equal to PrivateKeyData: %d instead of %zu", recv_size,
                  sizeof(PrivateKeyData));
        return false;
    }
}

static bool confirm_private_key(PrivateKeyData* our_priv_key_data,
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
        // Make sure that they used the same IV as us
        if (memcmp(our_priv_key_data->iv, our_signed_priv_key_data->iv, HMAC_SIZE) != 0) {
            LOG_ERROR("Could not confirmPrivateKey: IV is incorrect!");
            return false;
        } else {
            // Calculate what the signature should have been,
            // By signing sig_data ourselves
            SignatureData sig_data;
            memcpy(sig_data.iv, our_priv_key_data->iv, sizeof(our_signed_priv_key_data->iv));
            memcpy(sig_data.private_key, private_key, sizeof(sig_data.private_key));
            char hmac_buffer[HMAC_SIZE];
            hmac(hmac_buffer, &sig_data, sizeof(sig_data), private_key);

            // And compare to
            if (memcmp(our_signed_priv_key_data->signature, hmac_buffer, HMAC_SIZE) != 0) {
                LOG_ERROR("Could not confirmPrivateKey: Verify HMAC Failed");
                return false;
            } else {
                return true;
            }
        }
    } else {
        LOG_ERROR("Recv Size was not equal to PrivateKeyData: %d instead of %zu", recv_size,
                  sizeof(PrivateKeyData));
        return false;
    }
}
