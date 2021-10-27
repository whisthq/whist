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

int tcp_ack(SocketAttributes* context);

/*
@brief                          Perform accept syscall and set fd to use flag
                                FD_CLOEXEC

@param sock_fd                  The socket file descriptor
@param sock_addr                The socket address
@param sock_len                 The size of the socket address

@returns                        The new socket file descriptor, -1 on failure
*/
SOCKET acceptp(SOCKET sock_fd, struct sockaddr* sock_addr, socklen_t* sock_len);


/**
 * @brief                          This will send a FractalPacket over TCP to
 *                                 the SocketAttributes context. This function does
 *                                 not create the packet from raw data, but
 *                                 assumes that the packet has been prepared by
 *                                 the caller (e.g. fragmented into
 *                                 appropriately-sized chunks by a fragmenter).
 *                                 This function assumes and checks that the
 *                                 packet is small enough to send without
 *                                 further breaking into smaller packets.
 *
 * @param context                  The socket context
 * @param packet                   A pointer to the packet to be sent
 * @param packet_size              The size of the packet to be sent
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int send_tcp_packet(SocketAttributes* context, FractalPacket* packet, size_t packet_size);

/**
 * @brief                          This will send a FractalPacket over TCP to
 *                                 the SocketAttributes context. A
 *                                 FractalPacketType is also provided to
 *                                 describe the packet
 *
 * @param context                  The socket context
 * @param type                     The FractalPacketType, either VIDEO, AUDIO,
 *                                 or MESSAGE
 * @param data                     A pointer to the data to be sent
 * @param len                      The number of bytes to send
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int send_tcp_packet_from_payload(SocketAttributes* context, FractalPacketType type, void* data,
                                 int len, int id);
                        
/**
 * @brief                          Initialize a TCP connection between a
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
 * @param recvfrom_timeout_s       The timeout that the SocketAttributes will use
 *                                 after being initialized
 * @param connection_timeout_ms    The timeout that will be used when attempting
 *                                 to connect. The handshake sends a few packets
 *                                 back and forth, so the upper
 *                                 bound of how long CreateXContext will take is
 *                                 some small constant times
 *                                 connection_timeout_ms
 * @param using_stun               True/false for whether or not to use the STUN server for this
 *                                 context
 * @param binary_aes_private_key   The AES private key used to encrypt the socket communication
 *
 * @returns                        Will return -1 on failure, will return 0 on
 *                                 success
 */
int create_tcp_context(SocketAttributes* context, char* destination, int port, int recvfrom_timeout_ms,
                       int stun_timeout_ms, bool using_stun, char* binary_aes_private_key);

/**
 * @brief                          Receive a FractalPacket from a SocketAttributes,
 *                                 if any such packet exists
 *
 * @param context                  The socket context
 * @param should_recvp             Only valid in TCP contexts. Adding in so that functions can
 *                                 be generalized.
 *                                 If false, this function will only pop buffered packets
 *                                 If true, this function will pull data from the TCP socket,
 *                                 but that might take a while
 *
 * @returns                        A pointer to the FractalPacket on success,
 *                                 NULL on failure
 */
FractalPacket* read_tcp_packet(SocketAttributes* context, bool should_recvp);

/**
 * @brief                          Frees a TCP/udp packet created by read_tcp_packet/read_udp_packet
 *
 * @param tcp_packet               The TCP/UDP packet to free
 */
void free_tcp_packet(FractalPacket* tcp_packet);

/**
 * @brief                          Split a payload into several packets approprately-sized
 *                                 for UDP transport, and write those files to a buffer.
 *
 * @param payload                  The payload data to be split into packets
 * @param payload_size             The size of the payload, in bytes
 * @param payload_id               An ID for the UDP data (must be positive)
 * @param packet_type              The FractalPacketType (video, audio, or message)
 * @param packet_buffer            The buffer to write the packets to
 * @param packet_buffer_length     The length of the packet buffer
 *
 * @returns                        The number of packets that were written to the buffer,
 *                                 or -1 on failure
 *
 * @note                           This function should be removed and replaced with
 *                                 a more general packet splitter/joiner context, which
 *                                 will enable us to use forward error correction, etc.
 */
int write_payload_to_tcp_packets(uint8_t* payload, size_t payload_size, int payload_id,
                             FractalPacketType packet_type, FractalPacket* packet_buffer,
                             size_t packet_buffer_length);

/*
============================
Public Function Implementations
============================
*/

bool create_tcp_socket_context(SocketContext* network_context, char* destination, int port,
                                           int recvfrom_timeout_s, int connection_timeout_ms,
                                           bool using_stun, char* binary_aes_private_key) {
    // Make a socket context
    network_context->context = safe_malloc(sizeof(SocketAttributes));

    // Initialize the SocketAttributes with using_stun as false
    if (create_tcp_context(network_context->context, destination, port, recvfrom_timeout_s, connection_timeout_ms,
                           using_stun, binary_aes_private_key) < 0) {
        LOG_WARNING("Failed to create TCP network context!");
        return false;
    };

    // Functions common to all network contexts
    network_context->ack = tcp_ack;

    // Funcitons common to only TCP contexts
    network_context->read_packet = read_tcp_packet;
    network_context->send_packet = send_tcp_packet;
    network_context->send_packet_from_payload = send_tcp_packet_from_payload;
    network_context->free_packet = free_tcp_packet;
    network_context->write_payload_to_packets = write_payload_to_tcp_packets;

    return true;
}

/*
============================
Private Function Implementations
============================
*/

int tcp_ack(SocketAttributes* context) {
    return sendp(context, NULL, 0);
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
        bool worked = get_last_network_error() == FRACTAL_EINPROGRESS;

        if (!worked) {
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

int create_tcp_server_context(SocketAttributes* context, int port, int recvfrom_timeout_ms,
                              int stun_timeout_ms) {
    /*
        Create a TCP server context

        Arguments:
            context (SocketAttributes*): pointer to the context to populate
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

int create_tcp_server_context_stun(SocketAttributes* context, int port, int recvfrom_timeout_ms,
                                   int stun_timeout_ms) {
    /*
        Create a TCP server context over STUN server

        Arguments:
            context (SocketAttributes*): pointer to the context to populate
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
        if ((single_recv_size = recvp(context, ((char*)&entry) + recv_size,
                                      max(0, (int)sizeof(entry) - recv_size))) < 0) {
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

int create_tcp_client_context(SocketAttributes* context, char* destination, int port,
                              int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a TCP client context

        Arguments:
            context (SocketAttributes*): pointer to the context to populate
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

int create_tcp_client_context_stun(SocketAttributes* context, char* destination, int port,
                                   int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a TCP client context over STUN server

        Arguments:
            context (SocketAttributes*): pointer to the context to populate
            destination (char*): the destination address
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
        if ((single_recv_size = recvp(context, ((char*)&entry) + recv_size,
                                      max(0, (int)sizeof(entry) - recv_size))) < 0) {
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

int create_tcp_context(SocketAttributes* context, char* destination, int port, int recvfrom_timeout_ms,
                       int stun_timeout_ms, bool using_stun, char* binary_aes_private_key) {
    /*
        Create a TCP context

        Arguments:
            context (SocketAttributes*): pointer to the context to populate
            destination (char*): the destination address
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection
            using_stun (bool): whether to connect over the STUN server or not
            binary_aes_private_key (char*): the AES private key to use for the connection

        Returns:
            (int): -1 on failure, 0 on success
    */

    if ((int)((unsigned short)port) != port) {
        LOG_ERROR("Port invalid: %d", port);
    }
    if (context == NULL) {
        LOG_ERROR("Context is NULL");
        return -1;
    }
    port = port_mappings[port];

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
            ret =
                create_tcp_server_context_stun(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            ret = create_tcp_client_context_stun(context, destination, port, recvfrom_timeout_ms,
                                                 stun_timeout_ms);
    } else {
        if (destination == NULL)
            ret = create_tcp_server_context(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            ret = create_tcp_client_context(context, destination, port, recvfrom_timeout_ms,
                                            stun_timeout_ms);
    }

    if (ret == -1) {
        return -1;
    }

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    return ret;
}

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

FractalPacket* read_tcp_packet(SocketAttributes* context, bool should_recvp) {
    /*
        Receive a FractalPacket from a SocketAttributes, if any such packet exists

        Arguments:
            context (SocketAttributes*): The socket context
            should_recvp (bool): If false, this function will only pop buffered packets
                If true, this function will pull data from the TCP socket,
                but that might take a while

        Returns:
            (FractalPacket*): A pointer to the FractalPacket on success, NULL on failure
    */

    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return NULL;
    }
    if (!context->is_tcp) {
        LOG_WARNING("TryReadingTCPPacket received a context that is NOT TCP!");
        return NULL;
    }
    // The dynamically sized buffer to read into
    DynamicBuffer* encrypted_tcp_packet_buffer = context->encrypted_tcp_packet_buffer;

#define TCP_SEGMENT_SIZE 4096

    int len = TCP_SEGMENT_SIZE;
    while (should_recvp && len == TCP_SEGMENT_SIZE) {
        // Make the tcp buffer larger if needed
        resize_dynamic_buffer(encrypted_tcp_packet_buffer,
                              context->reading_packet_len + TCP_SEGMENT_SIZE);
        // Try to fill up the buffer, in chunks of TCP_SEGMENT_SIZE
        len = recvp(context, encrypted_tcp_packet_buffer->buf + context->reading_packet_len,
                    TCP_SEGMENT_SIZE);

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

        // If the previous recvp was maxed out, ie == TCP_SEGMENT_SIZE,
        // then try pulling some more from recvp
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

void free_tcp_packet(FractalPacket* tcp_packet) {
    /*
        Frees a TCP packet created by read_tcp_packet

        Arguments:
            tcp_packet (FractalPacket*): The TCP packet to free
    */

    deallocate_region(tcp_packet);
}

int send_tcp_packet(SocketAttributes* context, FractalPacket* packet, size_t packet_size) {
    LOG_FATAL("send_tcp_packet has not been implemented!");
    return -1;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int send_tcp_packet_from_payload(SocketAttributes* context, FractalPacketType type, void* data,
                                 int len, int id) {
    /*
        This will send a FractalPacket over TCP to the SocketAttributes context containing
        the specified payload. A FractalPacketType is also provided to describe the packet.

        Arguments:
            context (SocketAttributes*): The socket context
            type (FractalPacketType): The FractalPacketType, either VIDEO, AUDIO, or MESSAGE
            data (void*): A pointer to the data to be sent
            len (int): The number of bytes to send

        Returns:
            (int): Will return -1 on failure, will return 0 on success
    */

    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return -1;
    }

    // Use 256kb static buffer for sending smaller TCP packets
    // But use our block allocator for sending large TCP packets
    // This function fragments the heap too much to use malloc here
    char* packet_buffer = allocate_region(sizeof(FractalPacket) + len + 64);
    char* encrypted_packet_buffer = allocate_region(sizeof(FractalPacket) + len + 128);

    FractalPacket* packet = (FractalPacket*)(sizeof(int) + packet_buffer);

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
    int unencrypted_len = get_packet_size(packet);
    int encrypted_len = encrypt_packet(packet, unencrypted_len,
                                       (FractalPacket*)(sizeof(int) + encrypted_packet_buffer),
                                       (unsigned char*)context->binary_aes_private_key);

    // Pass the length of the packet as the first byte
    *((int*)packet_buffer) = unencrypted_len;
    *((int*)encrypted_packet_buffer) = encrypted_len;

    // For now, the TCP network throttler is NULL, so this is a no-op.
    network_throttler_wait_byte_allocation(context->network_throttler, (size_t)encrypted_len);

    //#if LOG_NETWORKING
    // This is useful enough to print, even outside of LOG_NETWORKING GUARDS
    LOG_INFO("Sending a FractalPacket of size %d over TCP", unencrypted_len);
    //#endif
    // Send the packet
    // Send unencrypted during dev mode, and encrypted otherwise
    void* packet_to_send = ENCRYPTING_PACKETS ? encrypted_packet_buffer : packet_buffer;
    int packet_to_send_len = ENCRYPTING_PACKETS ? encrypted_len : unencrypted_len;
    bool failed = false;
    int ret = sendp(context, packet_to_send, sizeof(int) + packet_to_send_len);
    if (ret < 0) {
        int error = get_last_network_error();
        LOG_WARNING("Unexpected TCP Packet Error: %d", error);
        failed = true;
    }

    deallocate_region(packet_buffer);
    deallocate_region(encrypted_packet_buffer);

    // Return success code
    return failed ? -1 : 0;
}

int write_payload_to_tcp_packets(uint8_t* payload, size_t payload_size, int payload_id,
                             FractalPacketType packet_type, FractalPacket* packet_buffer,
                             size_t packet_buffer_length) {
    /*
        Split a payload into several packets approprately-sized
        for TCP transport, and write those files to a buffer.

        Arguments:
            payload (uint8_t*): The payload data to be split into packets
            payload_size (size_t): The size of the payload, in bytes
            payload_id (int): An ID for the TCP data (must be positive)
            packet_type (FractalPacketType): The FractalPacketType (video, audio, or message)
            packet_buffer (FractalPacket*): The buffer to write the packets to
            packet_buffer_length (size_t): The length of the packet buffer

        Returns:
            (int): The number of packets that were written to the buffer,
                or -1 on failure

        Note:
            This function should be removed and replaced with
            a more general packet splitter/joiner context, which
            will enable us to use forward error correction, etc.
    */
    LOG_FATAL("write_payload_to_tcp_packets has not been implemented!");
    return -1;
}