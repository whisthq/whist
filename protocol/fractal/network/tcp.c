#include "tcp.h"
#include <fractal/utils/aes.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

extern unsigned short port_mappings[USHRT_MAX + 1];

/*
============================
Private Functions
============================
*/

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
SOCKET acceptp(SOCKET sock_fd, struct sockaddr* sock_addr, socklen_t* sock_len);

/**
 * @brief                           Connect to a TCP Server
 *
 * @param sock_fd                   The socket file descriptor
 * @param sock_addr                 The socket address
 * @param timeout_ms                The timeout for the connection attempt
 *
 * @returns                         Returns true on success, false on failure.
 */
bool tcp_connect(SOCKET sock_fd, struct sockaddr_in sock_addr, int timeout_ms);

/*
============================
TCP Implementation of Network.h Interface
============================
*/

int tcp_ack(void* raw_context) {
    SocketContextData* context = raw_context;
    return sendp(context, NULL, 0);
}

FractalPacket* tcp_read_packet(void* raw_context, bool should_recv) {
    SocketContextData* context = raw_context;

    // The dynamically sized buffer to read into
    DynamicBuffer* encrypted_tcp_packet_buffer = context->encrypted_tcp_packet_buffer;

#define TCP_SEGMENT_SIZE 4096

    int len = TCP_SEGMENT_SIZE;
    while (should_recv && len == TCP_SEGMENT_SIZE) {
        // Make the tcp buffer larger if needed
        resize_dynamic_buffer(encrypted_tcp_packet_buffer,
                              context->reading_packet_len + TCP_SEGMENT_SIZE);
        // Try to fill up the buffer, in chunks of TCP_SEGMENT_SIZE
        len = recv(context->socket, encrypted_tcp_packet_buffer->buf + context->reading_packet_len,
                   TCP_SEGMENT_SIZE, 0);

        if (len < 0) {
            int err = get_last_network_error();
            if (err == FRACTAL_ETIMEDOUT || err == FRACTAL_EAGAIN) {
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

    if ((unsigned long)context->reading_packet_len >= sizeof(int)) {
        // The amount of data bytes read (actual len), and the amount of bytes
        // we're looking for (target len), respectively
        int actual_len = context->reading_packet_len - sizeof(int);
        int target_len = *((int*)encrypted_tcp_packet_buffer->buf);

        // If the target len is valid, and actual len > target len, then we're
        // good to go
        if (target_len >= 0 && actual_len >= target_len) {
            FractalPacket* decrypted_packet_buffer = allocate_region(target_len);
            int decrypted_len;
            if (ENCRYPTING_PACKETS) {
                // Decrypt the packet
                decrypted_len = decrypt_packet_n(
                    (FractalPacket*)(encrypted_tcp_packet_buffer->buf + sizeof(int)), target_len,
                    decrypted_packet_buffer, target_len,
                    (unsigned char*)context->binary_aes_private_key);
            } else {
                // The decrypted packet is just the original packet, during dev mode
                decrypted_len = target_len;
                memcpy(decrypted_packet_buffer, (encrypted_tcp_packet_buffer->buf + sizeof(int)),
                       target_len);
            }
#if LOG_NETWORKING
            LOG_INFO("Received a FractalPacket of size %d over TCP", decrypted_len);
#endif

            // Move the rest of the read bytes to the beginning of the buffer to
            // continue
            int start_next_bytes = sizeof(int) + target_len;
            for (unsigned long i = start_next_bytes; i < sizeof(int) + actual_len; i++) {
                encrypted_tcp_packet_buffer->buf[i - start_next_bytes] =
                    encrypted_tcp_packet_buffer->buf[i];
            }
            context->reading_packet_len = actual_len - target_len;

            // Realloc the buffer smaller if we have room to
            resize_dynamic_buffer(encrypted_tcp_packet_buffer, context->reading_packet_len);

            if (decrypted_len < 0) {
                // A warning not an error, since it doesn't simply we did something wrong
                // Someone else in the same coffeeshop as you could make you generate these
                // by sending bad TCP packets. After this point though, the packet is authenticated,
                // and problems with its data should be LOG_ERROR'ed
                LOG_WARNING("Could not decrypt TCP message");
                deallocate_region(decrypted_packet_buffer);
                return NULL;
            } else {
                // Return the decrypted packet
                return decrypted_packet_buffer;
            }
        }
    }

    return NULL;
}

void tcp_free_packet(void* raw_context, FractalPacket* tcp_packet) {
    /*
        Frees a TCP packet created by tcp_read_packet

        Arguments:
            tcp_packet (FractalPacket*): The TCP packet to free
    */

    deallocate_region(tcp_packet);
}

int tcp_send_constructed_packet(void* raw_context, FractalPacket* packet, size_t packet_size) {
    SocketContextData* context = raw_context;

    // Allocate a buffer for the encrypted packet
    char* encrypted_packet_buffer = allocate_region(packet_size + 128);

    // Encrypt the packet using aes encryption, offset by 1 byte
    int unencrypted_len = get_packet_size(packet);
    int encrypted_len = encrypt_packet(packet, unencrypted_len,
                                       (FractalPacket*)(sizeof(int) + encrypted_packet_buffer),
                                       (unsigned char*)context->binary_aes_private_key);

    // If we shouldn't be encrypted, then unencrypt the packet
    if (!ENCRYPTING_PACKETS) {
        encrypted_len = unencrypted_len;
        memcpy(encrypted_packet_buffer, packet, packet_size);
    }

    // Pass the length of the packet as the first byte
    *((int*)encrypted_packet_buffer) = encrypted_len;

    // For now, the TCP network throttler is NULL, so this is a no-op.
    network_throttler_wait_byte_allocation(context->network_throttler, (size_t)encrypted_len);

    //#if LOG_NETWORKING
    // This is useful enough to print, even outside of LOG_NETWORKING GUARDS
    LOG_INFO("Sending a FractalPacket of size %d over TCP", unencrypted_len);
    //#endif

    // Send the packet
    bool failed = false;
    int ret = sendp(context, encrypted_packet_buffer, sizeof(int) + encrypted_len);
    if (ret < 0) {
        int error = get_last_network_error();
        LOG_WARNING("Unexpected TCP Packet Error: %d", error);
        failed = true;
    }

    // Free the encrypted allocation
    deallocate_region(encrypted_packet_buffer);

    return failed ? -1 : 0;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int tcp_send_packet(void* raw_context, FractalPacketType type, void* data, int len, int id) {
    SocketContextData* context = raw_context;

    if (id != -1) {
        LOG_ERROR("ID should be -1 when sending over TCP!");
    }

    // Use our block allocator
    // This function fragments the heap too much to use malloc here
    int packet_size = sizeof(FractalPacket) + len;
    FractalPacket* packet = allocate_region(packet_size);

    // Contruct packet metadata
    packet->id = id;
    packet->type = type;
    packet->index = 0;
    packet->payload_size = len;
    packet->num_indices = 1;
    packet->is_a_nack = false;

    // Copy packet data
    memcpy(packet->data, data, len);

    // Send the packet
    int ret = tcp_send_constructed_packet(context, packet, packet_size);

    // Free the packet
    deallocate_region(packet);

    // Return success code
    return ret;
}

void tcp_destroy_socket_context(void* raw_context) {
    SocketContextData* context = raw_context;

    closesocket(context->socket);
    fractal_destroy_mutex(context->mutex);
    free_dynamic_buffer(context->encrypted_tcp_packet_buffer);
    free(context);
}

/*
============================
Private Function Implementations
============================
*/

SOCKET acceptp(SOCKET sock_fd, struct sockaddr* sock_addr, socklen_t* sock_len) {
    /*
        Accept a connection on `sock_fd` and return a new socket fd

        Arguments:
            sock_fd (SOCKET): file descriptor of socket that we are accepting a connection on
            sock_addr (struct sockaddr*): the address of the socket
            sock_len (socklen_t*): the length of the socket address struct

        Return:
            SOCKET: new fd for socket on success; INVALID_SOCKET on failure
    */

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
    /*
        Connect to TCP server

        Arguments:
            socket (SOCKET): socket to connect over
            addr (struct sockaddr_in): connection address information
            timeout_ms (int): timeout in milliseconds

        Returns:
            (bool): true on success, false on failure
    */

    // Connect to TCP server
    int ret;
    // Set to nonblocking
    set_timeout(socket, 0);
    // Following instructions here: https://man7.org/linux/man-pages/man2/connect.2.html
    // Observe the paragraph under EINPROGRESS for how to nonblocking connect over TCP
    if ((ret = connect(socket, (struct sockaddr*)(&addr), sizeof(addr))) < 0) {
        // EINPROGRESS is a valid error, anything else is invalid
        if (get_last_network_error() != FRACTAL_EINPROGRESS) {
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

int create_tcp_server_context(SocketContextData* context, int port, int recvfrom_timeout_ms,
                              int stun_timeout_ms) {
    /*
        Create a TCP server context

        Arguments:
            context (SocketContextData*): pointer to the context to populate
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection

        Returns:
            (int): -1 on failure, 0 on success
    */

    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    context->is_tcp = true;

    int opt;

    // Create TCP socket
    LOG_INFO("Creating TCP Socket");
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);
    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        closesocket(context->socket);
        return -1;
    }

    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    // Bind to port
    if (bind(context->socket, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    // Set listen queue
    LOG_INFO("Waiting for TCP Connection");
    if (listen(context->socket, 3) < 0) {
        LOG_WARNING("Could not listen(2)! %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    fd_set fd_read, fd_write;
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_SET(context->socket, &fd_read);
    FD_SET(context->socket, &fd_write);

    struct timeval tv;
    tv.tv_sec = stun_timeout_ms / MS_IN_SECOND;
    tv.tv_usec = (stun_timeout_ms % MS_IN_SECOND) * 1000;

    int ret;
    if ((ret = select((int)context->socket + 1, &fd_read, &fd_write, NULL,
                      stun_timeout_ms > 0 ? &tv : NULL)) <= 0) {
        if (ret == 0) {
            LOG_INFO("No TCP Connection Retrieved, ending TCP connection attempt.");
        } else {
            LOG_WARNING("Could not select! %d", get_last_network_error());
        }
        closesocket(context->socket);
        return -1;
    }

    // Accept connection from client
    LOG_INFO("Accepting TCP Connection");
    socklen_t slen = sizeof(context->addr);
    SOCKET new_socket;
    if ((new_socket = acceptp(context->socket, (struct sockaddr*)(&context->addr), &slen)) ==
        INVALID_SOCKET) {
        return -1;
    }

    LOG_INFO("PORT: %d", ntohs(context->addr.sin_port));

    closesocket(context->socket);
    context->socket = new_socket;

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
}

int create_tcp_server_context_stun(SocketContextData* context, int port, int recvfrom_timeout_ms,
                                   int stun_timeout_ms) {
    /*
        Create a TCP server context over STUN server

        Arguments:
            context (SocketContextData*): pointer to the context to populate
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection

        Returns:
            (int): -1 on failure, 0 on success
    */

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
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Create UDP socket
    SOCKET udp_s = socketp_udp();
    if (udp_s == INVALID_SOCKET) {
        return -1;
    }

    // cppcheck-suppress nullPointer
    sendto(udp_s, NULL, 0, 0, (struct sockaddr*)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);

    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    struct sockaddr_in origin_addr;
    // Connect over TCP to STUN
    LOG_INFO("Connecting to STUN TCP...");
    if (!tcp_connect(context->socket, stun_addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to STUN Server over TCP");
        return -1;
    }

    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->socket, (struct sockaddr*)&origin_addr, &slen) < 0) {
        LOG_WARNING("Could not get sock name");
        closesocket(context->socket);
        return -1;
    }

    // Send STUN request
    StunRequest stun_request = {0};
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        LOG_WARNING("Could not send STUN request to connected STUN server!");
        closesocket(context->socket);
        return -1;
    }

    // Receive STUN response
    clock t;
    start_timer(&t);

    int recv_size = 0;
    StunEntry entry = {0};

    while (recv_size < (int)sizeof(entry) && get_timer(t) < stun_timeout_ms) {
        int single_recv_size;
        if ((single_recv_size = recv(context->socket, ((char*)&entry) + recv_size,
                                     max(0, (int)sizeof(entry) - recv_size), 0)) < 0) {
            LOG_WARNING("Did not receive STUN response %d\n", get_last_network_error());
            closesocket(context->socket);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->socket);
        return -1;
    }

    // Print STUN response
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = entry.ip;
    client_addr.sin_port = entry.private_port;
    LOG_INFO("TCP STUN notified of desired request from %s:%d\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));

    closesocket(context->socket);

    // Create TCP socket
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    if (context->socket <= 0) {  // Windows & Unix cases
        LOG_WARNING("Could not create TCP socket %d\n", get_last_network_error());
        return -1;
    }

    opt = 1;
    if (setsockopt(context->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Bind to port
    if (bind(context->socket, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("WAIT");

    // Connect to client
    if (!tcp_connect(context->socket, client_addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to Client over TCP");
        return -1;
    }

    context->addr = client_addr;
    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));
    set_timeout(context->socket, recvfrom_timeout_ms);
    return 0;
}

int create_tcp_client_context(SocketContextData* context, char* destination, int port,
                              int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a TCP client context

        Arguments:
            context (SocketContextData*): pointer to the context to populate
            destination (char*): the destination address
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection

        Returns:
            (int): -1 on failure, 0 on success
    */

    UNUSED(stun_timeout_ms);
    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }
    if (destination == NULL) {
        LOG_WARNING("destination is NULL");
        return -1;
    }
    context->is_tcp = true;

    // Create TCP socket
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    LOG_INFO("Connecting to server...");

    fractal_sleep(200);

    // Connect to TCP server
    if (!tcp_connect(context->socket, context->addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to server over TCP");
        return -1;
    }

    LOG_INFO("Connected on %s:%d!\n", destination, port);

    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
}

int create_tcp_client_context_stun(SocketContextData* context, char* destination, int port,
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
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Tell the STUN to use UDP
    SOCKET udp_s = socketp_udp();
    if (udp_s == INVALID_SOCKET) {
        return -1;
    }

    // cppcheck-suppress nullPointer
    sendto(udp_s, NULL, 0, 0, (struct sockaddr*)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);
    // Client connection protocol
    context->is_server = false;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Connect to STUN server
    if (!tcp_connect(context->socket, stun_addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to STUN Server over TCP");
        return -1;
    }

    struct sockaddr_in origin_addr;
    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->socket, (struct sockaddr*)&origin_addr, &slen) < 0) {
        LOG_WARNING("Could not get sock name");
        closesocket(context->socket);
        return -1;
    }

    // Make STUN request
    StunRequest stun_request = {0};
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        LOG_WARNING("Could not send STUN request to connected STUN server!");
        closesocket(context->socket);
        return -1;
    }

    // Receive STUN response
    clock t;
    start_timer(&t);

    int recv_size = 0;
    StunEntry entry = {0};

    while (recv_size < (int)sizeof(entry) && get_timer(t) < stun_timeout_ms) {
        int single_recv_size;
        if ((single_recv_size = recv(context->socket, ((char*)&entry) + recv_size,
                                     max(0, (int)sizeof(entry) - recv_size), 0)) < 0) {
            LOG_WARNING("Did not receive STUN response %d\n", get_last_network_error());
            closesocket(context->socket);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("STUN Response of wrong size! %d", recv_size);
        closesocket(context->socket);
        return -1;
    } else if (entry.ip != stun_request.entry.ip ||
               entry.public_port != stun_request.entry.public_port) {
        LOG_WARNING("STUN Response IP and/or Public Port is incorrect!");
        closesocket(context->socket);
        return -1;
    } else if (entry.private_port == 0) {
        LOG_WARNING("STUN reported no such IP Address");
        closesocket(context->socket);
        return -1;
    } else {
        LOG_WARNING("Received STUN response! Public %d is mapped to private %d\n",
                    ntohs((unsigned short)entry.public_port),
                    ntohs((unsigned short)entry.private_port));
        context->addr.sin_family = AF_INET;
        context->addr.sin_addr.s_addr = entry.ip;
        context->addr.sin_port = entry.private_port;
    }

    // Print STUN response
    struct in_addr a;
    a.s_addr = entry.ip;
    LOG_WARNING("TCP STUN responded that the TCP server is located at %s:%d\n", inet_ntoa(a),
                ntohs(entry.private_port));

    closesocket(context->socket);

    // Create TCP socket
    if ((context->socket = socketp_tcp()) == INVALID_SOCKET) {
        return -1;
    }

    // Reuse addr
    opt = 1;
    if (setsockopt(context->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        LOG_WARNING("Could not setsockopt SO_REUSEADDR");
        return -1;
    }

    // Bind to port
    if (bind(context->socket, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }
    set_timeout(context->socket, stun_timeout_ms);

    LOG_INFO("Connecting to server...");

    // Connect to TCP server
    if (!tcp_connect(context->socket, context->addr, stun_timeout_ms)) {
        LOG_WARNING("Could not connect to server over TCP");
        return -1;
    }

    LOG_INFO("Connected on %s:%d!\n", destination, port);

    set_timeout(context->socket, recvfrom_timeout_ms);
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

bool create_tcp_socket_context(SocketContext* network_context, char* destination, int port,
                               int recvfrom_timeout_ms, int connection_timeout_ms, bool using_stun,
                               char* binary_aes_private_key) {
    // Populate function pointer table
    network_context->ack = tcp_ack;
    network_context->read_packet = tcp_read_packet;
    network_context->free_packet = tcp_free_packet;
    network_context->send_packet = tcp_send_packet;
    network_context->destroy_socket_context = tcp_destroy_socket_context;

    // Create the SocketContextData, and set to zero
    SocketContextData* context = safe_malloc(sizeof(SocketContextData));
    memset(context, 0, sizeof(SocketContextData));
    network_context->context = context;

    // Map Port
    if ((int)((unsigned short)port) != port) {
        LOG_ERROR("Port invalid: %d", port);
    }
    port = port_mappings[port];

    // Initialize data
    context->timeout = recvfrom_timeout_ms;
    context->mutex = fractal_create_mutex();
    memcpy(context->binary_aes_private_key, binary_aes_private_key,
           sizeof(context->binary_aes_private_key));
    context->reading_packet_len = 0;
    context->encrypted_tcp_packet_buffer = init_dynamic_buffer(true);
    resize_dynamic_buffer(context->encrypted_tcp_packet_buffer, 0);
    context->network_throttler = NULL;

    int ret;

    if (using_stun) {
        if (destination == NULL)
            ret = create_tcp_server_context_stun(context, port, recvfrom_timeout_ms,
                                                 connection_timeout_ms);
        else
            ret = create_tcp_client_context_stun(context, destination, port, recvfrom_timeout_ms,
                                                 connection_timeout_ms);
    } else {
        if (destination == NULL)
            ret = create_tcp_server_context(context, port, recvfrom_timeout_ms,
                                            connection_timeout_ms);
        else
            ret = create_tcp_client_context(context, destination, port, recvfrom_timeout_ms,
                                            connection_timeout_ms);
    }

    if (ret == -1) {
        return false;
    }

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        free(network_context->context);
        return false;
    }

    return true;
}