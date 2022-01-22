#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, needs to
                                         // be at the very top
#endif

/*
============================
Includes
============================
*/

#include "tcp.h"
#include <whist/utils/aes.h>
#include <whist/utils/clock.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

/*
============================
Defines
============================
*/

// A TCPPacket that gets sent over the network
typedef struct {
    AESMetadata aes_metadata;
    int payload_size;
    char payload[];
} TCPPacket;

// Get tcp packet size from a TCPPacket*
#define get_tcp_packet_size(tcp_packet) ((size_t)(sizeof(TCPPacket) + (tcp_packet)->payload_size))

typedef struct {
    int timeout;
    SOCKET socket;
    struct sockaddr_in addr;
    WhistMutex mutex;
    char binary_aes_private_key[16];
    // Used for reading TCP packets
    int reading_packet_len;
    DynamicBuffer* encrypted_tcp_packet_buffer;
    NetworkThrottleContext* network_throttler;

    WhistTimer last_ack_timer;
} TCPContext;

/*
============================
Globals
============================
*/

// TODO: Remove `extern` globals
extern unsigned short port_mappings[USHRT_MAX + 1];

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Create a TCP Server Context,
 *                                 which will wait connection_timeout_ms
 *                                 for a client to connect.
 *
 * @param context                  The TCP context to connect with
 * @param port                     The port to open up
 * @param connection_timeout_ms    The amount of time to wait for a client
 *
 * @returns                        -1 on failure, 0 on success
 *
 * @note                           This may overwrite the timeout on context->socket,
 *                                 Use set_timeout to restore it
 */
int create_tcp_server_context(TCPContext* context, int port, int connection_timeout_ms);

/**
 * @brief                          Create a TCP Client Context,
 *                                 which will try to connect for connection_timeout_ms
 *
 * @param context                  The TCP context to connect with
 * @param destination              The IP address to connect to
 * @param port                     The port to connect to
 * @param connection_timeout_ms    The amount of time to try to connect
 *
 * @returns                        -1 on failure, 0 on success
 *
 * @note                           This may overwrite the timeout on context->socket,
 *                                 Use set_timeout to restore it
 */
int create_tcp_client_context(TCPContext* context, char* destination, int port,
                              int connection_timeout_ms);

/**
 * @brief                          Sends a fully constructed WhistPacket
 *
 * @param context                  The TCP Context
 * 
 * @param packet                   The packet to send
 *
 * @returns                        Will return -1 on failure, and 0 on success
 *                                 Failure implies that the socket is
 *                                 broken or the TCP connection has ended, use
 *                                 get_last_network_error() to learn more about the
 *                                 error
 */
int tcp_send_constructed_packet(TCPContext* context, WhistPacket* packet);

/**
 * @brief                          Perform socket() syscall and set fds to
 *                                 use flag FD_CLOEXEC
 *
 * @returns                        The socket file descriptor, -1 on failure
 */
static SOCKET socketp_tcp(void);

/**
 * @brief                           Perform accept syscall and set fd to use flag
 *                                  FD_CLOEXEC
 *
 * @param sock_fd                   The socket file descriptor
 * @param sock_addr                 The socket address
 * @param sock_len                  The size of the socket address
 *
 * @returns                         The new socket file descriptor, -1 on failure
 */
static SOCKET acceptp(SOCKET sock_fd, struct sockaddr* sock_addr, socklen_t* sock_len);

/**
 * @brief                           Connect to a TCP Server
 *
 * @param sock_fd                   The socket file descriptor
 * @param sock_addr                 The socket address
 * @param timeout_ms                The timeout for the connection attempt
 *
 * @returns                         Returns true on success, false on failure.
 */
static bool tcp_connect(SOCKET sock_fd, struct sockaddr_in sock_addr, int timeout_ms);

/*
============================
TCP Implementation of Network.h Interface
============================
*/

bool tcp_update(void* raw_context) {
    TCPContext* context = raw_context;

    int ret = 0;

    // Keep sending a length-0 packet every 50 ms,
    // To keep the TCP connection alive and additionally check
    // whether or not it is still alive
    if (get_timer(&context->last_ack_timer) > 50.0 / MS_IN_SECOND) {
        ret = send(context->socket, NULL, 0, 0);
        start_timer(&context->last_ack_timer);
    }

    return ret >= 0;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int tcp_send_packet(void* raw_context, WhistPacketType type, void* data, int len, int id, bool start_of_stream) {
    TCPContext* context = raw_context;
    UNUSED(start_of_stream);

    if (id != -1) {
        LOG_ERROR("ID should be -1 when sending over TCP!");
    }

    // Use our block allocator
    // This function fragments the heap too much to use malloc here
    int packet_size = PACKET_HEADER_SIZE + len;
    WhistPacket* packet = allocate_region(packet_size);

    // Contruct packet metadata
    packet->id = id;
    packet->type = type;
    packet->payload_size = len;

    // Copy packet data, verifying the packetsize first
    FATAL_ASSERT(get_packet_size(packet) == packet_size);
    memcpy(packet->data, data, len);

    // Send the packet
    int ret = tcp_send_constructed_packet(context, packet);

    // Free the packet
    deallocate_region(packet);

    // Return success code
    return ret;
}

void* tcp_get_packet(void* raw_context, WhistPacketType packet_type) {
    TCPContext* context = raw_context;

    // The dynamically sized buffer to read into
    DynamicBuffer* encrypted_tcp_packet_buffer = context->encrypted_tcp_packet_buffer;

#define TCP_SEGMENT_SIZE 4096

    int len = TCP_SEGMENT_SIZE;
    while (len == TCP_SEGMENT_SIZE) {
        // Make the tcp buffer larger if needed
        resize_dynamic_buffer(encrypted_tcp_packet_buffer,
                              context->reading_packet_len + TCP_SEGMENT_SIZE);
        // Try to fill up the buffer, in chunks of TCP_SEGMENT_SIZE
        len = recv_no_intr(context->socket,
                           encrypted_tcp_packet_buffer->buf + context->reading_packet_len,
                           TCP_SEGMENT_SIZE, 0);

        if (len < 0) {
            int err = get_last_network_error();
            if (err == WHIST_ETIMEDOUT || err == WHIST_EAGAIN) {
            } else {
                LOG_WARNING("Network Error %d", err);
            }
        } else if (len > 0) {
            // LOG_INFO( "READ LEN: %d", len );
            context->reading_packet_len += len;
        }

        // If the previous recv was maxed out, ie == TCP_SEGMENT_SIZE,
        // then try pulling some more from recv
    };

    // If we haven't decoded a packet,
    // and we have enough bytes to read another TCPPacket header...
    if ((unsigned long)context->reading_packet_len >= sizeof(TCPPacket)) {
        // Get a pointer to the tcp_packet
        TCPPacket* tcp_packet = (TCPPacket*)encrypted_tcp_packet_buffer->buf;

        // TODO: An untrusted party could make tcp_packet->payload_size so large that it overflows
        // This should be fixed
        int tcp_packet_size = get_tcp_packet_size(tcp_packet);

        // If the target len is valid (Checking because this is an untrusted network),
        // and we've read enough bytes for the whole tcp packet,
        // we're ready to go
        if (tcp_packet_size >= 0 && context->reading_packet_len >= tcp_packet_size) {
            // The resulting packet will be <= the encrypted size
            WhistPacket* whist_packet = allocate_region(tcp_packet->payload_size);

            if (ENCRYPTING_PACKETS) {
                // Decrypt into whist_packet
                int decrypted_len = decrypt_packet(
                    whist_packet, tcp_packet->payload_size, tcp_packet->aes_metadata,
                    tcp_packet->payload, tcp_packet->payload_size, context->binary_aes_private_key);
                if (decrypted_len == -1) {
                    // Deallocate and prepare to return NULL on decryption failure
                    LOG_WARNING("Could not decrypt TCP message");
                    deallocate_region(whist_packet);
                    whist_packet = NULL;
                } else {
                    // Verify that the length matches what the WhistPacket's length should be
                    FATAL_ASSERT(decrypted_len == get_packet_size(whist_packet));
                }
            } else {
                // If we're not encrypting packets, just copy it over
                memcpy(whist_packet, tcp_packet->payload, tcp_packet->payload_size);
                // Verify that the length matches what the WhistPacket's length should be
                FATAL_ASSERT(tcp_packet->payload_size == get_packet_size(whist_packet));
            }

#if LOG_NETWORKING
            LOG_INFO("Received a WhistPacket of size %d over TCP", decrypted_len);
#endif

            // Move the rest of the already read bytes,
            // to the beginning of the buffer to continue
            for (int i = tcp_packet_size; i < context->reading_packet_len; i++) {
                encrypted_tcp_packet_buffer->buf[i - tcp_packet_size] =
                    encrypted_tcp_packet_buffer->buf[i];
            }
            context->reading_packet_len -= tcp_packet_size;

            // Realloc the buffer smaller if we have room to
            resize_dynamic_buffer(encrypted_tcp_packet_buffer, context->reading_packet_len);

            // Return the whist_packet, which will be NULL if decoding failed
            if (whist_packet->type == packet_type) {
                return whist_packet;
            } else {
                deallocate_region(whist_packet);
                LOG_ERROR("Got a TCP whist packet of type that didn't match %d! %d", (int)packet_type, (int)whist_packet->type);
                return NULL;
            }
        }
    }

    return NULL;
}

void tcp_free_packet(void* raw_context, WhistPacket* tcp_packet) {
    UNUSED(raw_context);
    deallocate_region(tcp_packet);
}

bool tcp_get_pending_stream_reset(void* raw_context, WhistPacketType packet_type) {
    TCPContext* context = raw_context;
    LOG_FATAL("Not implemented for TCP yet!");
}

void tcp_destroy_socket_context(void* raw_context) {
    TCPContext* context = raw_context;

    closesocket(context->socket);
    whist_destroy_mutex(context->mutex);
    free_dynamic_buffer(context->encrypted_tcp_packet_buffer);
    free(context);
}

/*
============================
Public Function Implementations
============================
*/

bool create_tcp_socket_context(SocketContext* network_context, char* destination, int port,
                               int recvfrom_timeout_ms, int connection_timeout_ms, bool using_stun,
                               char* binary_aes_private_key) {
    /*
        Create a TCP socket context

        Arguments:
            context (SocketContext*): pointer to the SocketContext struct to initialize
            destination (char*): the destination address, NULL means act as a server
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            connection_timeout_ms (int): timeout, in milliseconds, for socket connection
            using_stun (bool): Whether or not to use STUN
            binary_aes_private_key (char*): The 16byte AES key to use

        Returns:
            (int): -1 on failure, 0 on success
    */

    // STUN isn't implemented
    FATAL_ASSERT(using_stun == false);

    // Populate function pointer table
    network_context->socket_update = tcp_update;
    network_context->send_packet = tcp_send_packet;
    network_context->get_packet = tcp_get_packet;
    network_context->free_packet = tcp_free_packet;
    network_context->get_pending_stream_reset = tcp_get_pending_stream_reset;
    network_context->destroy_socket_context = tcp_destroy_socket_context;

    // Create the TCPContext, and set to zero
    TCPContext* context = safe_malloc(sizeof(TCPContext));
    memset(context, 0, sizeof(TCPContext));
    network_context->context = context;

    // if dest is NULL, it means the context will be listening for income connections
    if (destination == NULL) {
        FATAL_ASSERT(network_context->listen_socket != NULL);
        /*
            for tcp, just make a copy of the socket, do not transfer ownership to TCPContext.
            when TCPContext is destoryed, the copied listen_socket should NOT be closed.
        */
        // TODO: This logic is very confusing,
        // This should be fixed
        context->socket = *network_context->listen_socket;
    }

    // Map Port
    if ((int)((unsigned short)port) != port) {
        LOG_ERROR("Port invalid: %d", port);
    }
    port = port_mappings[port];

    // Initialize data
    context->timeout = recvfrom_timeout_ms;
    context->mutex = whist_create_mutex();
    memcpy(context->binary_aes_private_key, binary_aes_private_key,
           sizeof(context->binary_aes_private_key));
    context->reading_packet_len = 0;
    context->encrypted_tcp_packet_buffer = init_dynamic_buffer(true);
    resize_dynamic_buffer(context->encrypted_tcp_packet_buffer, 0);
    context->network_throttler = NULL;
    start_timer(&context->last_ack_timer);

    int ret;

    if (destination == NULL) {
        ret = create_tcp_server_context(context, port, connection_timeout_ms);
    } else {
        ret = create_tcp_client_context(context, destination, port, connection_timeout_ms);
    }

    if (ret == -1) {
        free(context);
        network_context->context = NULL;
        return false;
    }

    // Restore the original timeout
    set_timeout(context->socket, context->timeout);

    return true;
}

int create_tcp_listen_socket(SOCKET* sock, int port, int timeout_ms) {
    LOG_INFO("Creating listen TCP Socket");
    *sock = socketp_tcp();
    if (*sock == INVALID_SOCKET) {
        LOG_ERROR("Failed to create TCP listen socket");
        return -1;
    }

    set_timeout(*sock, timeout_ms);

    // Reuse addr
    int opt = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        LOG_ERROR("Could not setsockopt SO_REUSEADDR");
        closesocket(*sock);
        return -1;
    }

    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    // Bind to port
    if (bind(*sock, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_ERROR("Failed to bind to port %d! %d\n", port, get_last_network_error());
        closesocket(*sock);
        return -1;
    }

    // Set listen queue
    LOG_INFO("Waiting for TCP Connection");
    if (listen(*sock, 3) < 0) {
        LOG_ERROR("Could not listen(2)! %d\n", get_last_network_error());
        closesocket(*sock);
        return -1;
    }

    return 0;
}

/*
============================
Private Function Implementations
============================
*/

int create_tcp_server_context(TCPContext* context, int port,
                              int connection_timeout_ms) {
    FATAL_ASSERT(context != NULL);

    fd_set fd_read, fd_write;
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_SET(context->socket, &fd_read);
    FD_SET(context->socket, &fd_write);

    struct timeval tv;
    tv.tv_sec = connection_timeout_ms / MS_IN_SECOND;
    tv.tv_usec = (connection_timeout_ms % MS_IN_SECOND) * 1000;

    int ret;
    if ((ret = select((int)context->socket + 1, &fd_read, &fd_write, NULL,
                      connection_timeout_ms > 0 ? &tv : NULL)) <= 0) {
        if (ret == 0) {
            LOG_INFO("No TCP Connection Retrieved, ending TCP connection attempt.");
        } else {
            LOG_WARNING("Could not select! %d", get_last_network_error());
        }
        return -1;
    }

    // Accept connection from client
    LOG_INFO("Waiting for TCP client on port %d...", port);
    socklen_t slen = sizeof(context->addr);
    SOCKET new_socket;
    if ((new_socket = acceptp(context->socket, (struct sockaddr*)(&context->addr), &slen)) ==
        INVALID_SOCKET) {
        LOG_WARNING("Could not accept() over TCP! %d", get_last_network_error());
        return -1;
    }

    context->socket = new_socket;

    // Handshake
    if (!handshake_private_key(context->socket, connection_timeout_ms, context->binary_aes_private_key)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Client received on %d from %s:%d over TCP!\n", port, inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    return 0;
}

int create_tcp_client_context(TCPContext* context, char* destination, int port,
                              int connection_timeout_ms) {
    FATAL_ASSERT(context != NULL);
    FATAL_ASSERT(destination != NULL);

    // Create TCP socket
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    // Client connection protocol

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    LOG_INFO("Connecting to server at %s:%d over TCP...", destination, port);

    // Connect to TCP server, waiting for connection_timeout_ms
    set_timeout(context->socket, connection_timeout_ms);
    if (!tcp_connect(context->socket, context->addr, connection_timeout_ms)) {
        LOG_WARNING("Could not connect to server over TCP");
        return -1;
    }

    // Handshake
    if (!handshake_private_key(context->socket, connection_timeout_ms, context->binary_aes_private_key)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Connected to %s:%d over TCP!", destination, port);

    return 0;
}


int tcp_send_constructed_packet(TCPContext* context, WhistPacket* packet) {
    int packet_size = get_packet_size(packet);

    // Allocate a buffer for the encrypted packet
    TCPPacket* tcp_packet =
        allocate_region(sizeof(TCPPacket) + packet_size + MAX_ENCRYPTION_SIZE_INCREASE);

    if (ENCRYPTING_PACKETS) {
        // If we're encrypting packets, encrypt the packet into tcp_packet
        int encrypted_len = encrypt_packet(tcp_packet->payload, &tcp_packet->aes_metadata, packet,
                                           packet_size, context->binary_aes_private_key);
        tcp_packet->payload_size = encrypted_len;
    } else {
        // Otherwise, just write it to tcp_packet directly
        tcp_packet->payload_size = packet_size;
        memcpy(tcp_packet->payload, packet, packet_size);
    }

    int tcp_packet_size = get_tcp_packet_size(tcp_packet);

    // For now, the TCP network throttler is NULL, so this is a no-op.
    network_throttler_wait_byte_allocation(context->network_throttler, tcp_packet_size);

    //#if LOG_NETWORKING
    // This is useful enough to print, even outside of LOG_NETWORKING GUARDS
    LOG_INFO("Sending a WhistPacket of size %d (Total %d bytes), over TCP", packet_size,
             tcp_packet_size);
    //#endif

    // Send the packet
    bool failed = false;
    int ret = send(context->socket, (const char*)tcp_packet, tcp_packet_size, 0);
    if (ret < 0) {
        int error = get_last_network_error();
        LOG_WARNING("Unexpected TCP Packet Error: %d", error);
        failed = true;
    }

    // Free the encrypted allocation
    deallocate_region(tcp_packet);

    return failed ? -1 : 0;
}

SOCKET socketp_tcp(void) {
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

SOCKET acceptp(SOCKET sock_fd, struct sockaddr* sock_addr, socklen_t* sock_len) {
#if defined(_GNU_SOURCE) && defined(SOCK_CLOEXEC)
    SOCKET new_socket = accept4(sock_fd, sock_addr, sock_len, SOCK_CLOEXEC);
#else
    // Accept connection from client
    SOCKET new_socket = accept(sock_fd, sock_addr, sock_len);
    if (new_socket < 0) {
        LOG_WARNING("Did not receive response from client! %d\n", get_last_network_error());
        return INVALID_SOCKET;
    }

#ifndef _WIN32
    // Set socket to close on child exec
    // Not necessary for windows because CreateProcessA creates an independent process
    if (fcntl(new_socket, F_SETFD, fcntl(new_socket, F_GETFD) | FD_CLOEXEC) < 0) {
        LOG_WARNING("Could not set fcntl to set socket to close on child exec");
        return INVALID_SOCKET;
    }
#endif
#endif

    return new_socket;
}

bool tcp_connect(SOCKET socket, struct sockaddr_in addr, int timeout_ms) {
    // Connect to TCP server
    int ret;
    // Set to nonblocking
    set_timeout(socket, 0);
    // Following instructions here: https://man7.org/linux/man-pages/man2/connect.2.html
    // Observe the paragraph under EINPROGRESS for how to nonblocking connect over TCP
    if ((ret = connect(socket, (struct sockaddr*)(&addr), sizeof(addr))) < 0) {
        // EINPROGRESS is a valid error, anything else is invalid
        if (get_last_network_error() != WHIST_EINPROGRESS) {
            LOG_WARNING(
                "Could not connect() over TCP to server: Returned %d, Error "
                "Code %d",
                ret, get_last_network_error());
            closesocket(socket);
            return false;
        }
    }

    // Select connection
    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / MS_IN_SECOND;
    tv.tv_usec = (timeout_ms % MS_IN_SECOND) * MS_IN_SECOND;
    if ((ret = select((int)socket + 1, NULL, &set, NULL, &tv)) <= 0) {
        if (ret == 0) {
            LOG_INFO("No TCP Connection Retrieved, ending TCP connection attempt.");
        } else {
            LOG_WARNING(
                "Could not select() over TCP to server: Returned %d, Error Code "
                "%d\n",
                ret, get_last_network_error());
        }
        closesocket(socket);
        return false;
    }

    // Check for errors that may have happened during the select()
    int error;
    socklen_t len = sizeof(error);
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (void*)&error, &len) < 0) {
        LOG_WARNING("Could not getsockopt SO_ERROR");
        closesocket(socket);
        return false;
    }
    if (error != 0) {
        LOG_WARNING("getsockopt has captured the following error: %d", error);
        closesocket(socket);
        return false;
    }

    set_timeout(socket, timeout_ms);
    return true;
}