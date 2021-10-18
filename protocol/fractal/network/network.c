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

#define STUN_IP "0.0.0.0"
#define STUN_PORT 48800

#define BITS_IN_BYTE 8.0
#define MS_IN_SECOND 1000

// Global data
unsigned short port_mappings[USHRT_MAX + 1];

/*
============================
Private Custom Types
============================
*/

typedef struct {
    unsigned int ip;
    unsigned short private_port;
    unsigned short public_port;
} StunEntry;

typedef enum StunRequestType { ASK_INFO, POST_INFO } StunRequestType;

typedef struct {
    StunRequestType type;
    StunEntry entry;
} StunRequest;

typedef struct {
    char iv[16];
    char signature[32];
} PrivateKeyData;

typedef struct {
    char iv[16];
    char private_key[16];
} SignatureData;

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
@brief                          This will set `socket` to have timeout
                                timeout_ms.

@param socket                   The SOCKET to be configured
@param timeout_ms               The maximum amount of time that all recv/send
                                calls will take for that socket (0 to return
                                immediately, -1 to never return). Set 0 to have
                                a non-blocking socket, and -1 for an indefinitely
                                blocking socket.
*/
void set_timeout(SOCKET socket, int timeout_ms);

/*
@brief                          Perform socket syscalls and set fds to
                                use flag FD_CLOEXEC

@returns                        The socket file descriptor, -1 on failure
*/
SOCKET socketp_tcp();
SOCKET socketp_udp();

/*
@brief                          Perform accept syscall and set fd to use flag
                                FD_CLOEXEC

@param sock_fd                  The socket file descriptor
@param sock_addr                The socket address
@param sock_len                 The size of the socket address

@returns                        The new socket file descriptor, -1 on failure
*/
SOCKET acceptp(SOCKET sock_fd, struct sockaddr* sock_addr, socklen_t* sock_len);

/*
@brief                          This will prepare the private key data

@param priv_key_data            The private key data buffer
*/
void prepare_private_key_request(PrivateKeyData* priv_key_data);

/*
@brief                          This will sign the other connection's private key data

@param priv_key_data            The private key data buffer
@param recv_size                The length of the buffer
@param private_key              The private key

@returns                        True if the verification succeeds, false if it fails
*/
bool sign_private_key(PrivateKeyData* priv_key_data, int recv_size, void* private_key);

/*
@brief                          This will verify the given private key

@param our_priv_key_data        The private key data buffer
@param our_signed_priv_key_data The signed private key data buffer
@param recv_size                The length of the buffer
@param private_key              The private key

@returns                        True if the verification succeeds, false if it fails
*/
bool confirm_private_key(PrivateKeyData* our_priv_key_data,
                         PrivateKeyData* our_signed_priv_key_data, int recv_size,
                         void* private_key);

/*
============================
Private Function Implementations
============================
*/

bool handshake_private_key(SocketContext* context) {
    /*
        Perform a private key handshake with a peer.

        Arguments:
            context (SocketContext*): the socket context to be used

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

int create_tcp_server_context(SocketContext* context, int port, int recvfrom_timeout_ms,
                              int stun_timeout_ms) {
    /*
        Create a TCP server context

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

int create_tcp_server_context_stun(SocketContext* context, int port, int recvfrom_timeout_ms,
                                   int stun_timeout_ms) {
    /*
        Create a TCP server context over STUN server

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

int create_tcp_client_context(SocketContext* context, char* destination, int port,
                              int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a TCP client context

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

int create_tcp_client_context_stun(SocketContext* context, char* destination, int port,
                                   int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a TCP client context over STUN server

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

int create_tcp_context(SocketContext* context, char* destination, int port, int recvfrom_timeout_ms,
                       int stun_timeout_ms, bool using_stun, char* binary_aes_private_key) {
    /*
        Create a TCP context

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

int create_udp_server_context(SocketContext* context, int port, int recvfrom_timeout_ms,
                              int stun_timeout_ms) {
    /*
        Create a UDP server context

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

    context->udp_is_connected = false;
    context->is_tcp = false;
    // Create UDP socket
    if ((context->socket = socketp_udp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);
    // Server connection protocol
    context->is_server = true;

    // Bind the server port to the advertized public port
    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    if (bind(context->socket, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_WARNING("Failed to bind to port! %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Waiting for client to connect to %s:%d...\n", "localhost", port);

    socklen_t slen = sizeof(context->addr);
    int recv_size;
    if ((recv_size = recvfrom(context->socket, NULL, 0, 0, (struct sockaddr*)(&context->addr),
                              &slen)) != 0) {
        LOG_WARNING("Failed to receive ack! %d %d", recv_size, get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
}

int create_udp_server_context_stun(SocketContext* context, int port, int recvfrom_timeout_ms,
                                   int stun_timeout_ms) {
    /*
        Create a UDP client context over STUN server

        Arguments:
            context (SocketContext*): pointer to the context to populate
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection

        Returns:
            (int): -1 on failure, 0 on success
    */

    context->udp_is_connected = false;
    context->is_tcp = false;

    // Create UDP socket
    if ((context->socket = socketp_udp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Server connection protocol
    context->is_server = true;

    // Tell the STUN to log our requested virtual port
    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);

    StunRequest stun_request = {0};
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    LOG_INFO("Sending stun entry to STUN...");
    if (sendto(context->socket, (const char*)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr*)&stun_addr, sizeof(stun_addr)) < 0) {
        LOG_WARNING("Could not send message to STUN %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Waiting for client to connect to %s:%d...\n", "localhost", port);

    // Receive client's connection attempt
    // Update the STUN every 100ms
    set_timeout(context->socket, 100);

    // But keep track of time to compare against stun_timeout_ms
    clock recv_timer;
    start_timer(&recv_timer);

    socklen_t slen = sizeof(context->addr);
    StunEntry entry = {0};
    int recv_size;
    while ((recv_size = recvfrom(context->socket, (char*)&entry, sizeof(entry), 0,
                                 (struct sockaddr*)(&context->addr), &slen)) < 0) {
        // If we haven't spent too much time waiting, and our previous 100ms
        // poll failed, then send another STUN update
        if (get_timer(recv_timer) * MS_IN_SECOND < stun_timeout_ms &&
            (get_last_network_error() == FRACTAL_ETIMEDOUT ||
             get_last_network_error() == FRACTAL_EAGAIN)) {
            if (sendto(context->socket, (const char*)&stun_request, sizeof(stun_request), 0,
                       (struct sockaddr*)&stun_addr, sizeof(stun_addr)) < 0) {
                LOG_WARNING("Could not send message to STUN %d\n", get_last_network_error());
                closesocket(context->socket);
                return -1;
            }
            continue;
        }
        LOG_WARNING("Did not receive response from client! %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    set_timeout(context->socket, 350);

    if (recv_size != sizeof(entry)) {
        LOG_WARNING("STUN response was not the size of an entry!");
        closesocket(context->socket);
        return -1;
    }

    // Setup addr to open up port
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = entry.ip;
    context->addr.sin_port = entry.private_port;

    LOG_INFO("Received STUN response, client connection desired from %s:%d\n",
             inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));

    // Open up the port
    if (sendp(context, NULL, 0) < 0) {
        LOG_ERROR("sendp(3) failed! Could not open up port! %d", get_last_network_error());
        return false;
    }
    fractal_sleep(150);

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }
    set_timeout(context->socket, recvfrom_timeout_ms);

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
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    return 0;
}

int create_udp_client_context(SocketContext* context, char* destination, int port,
                              int recvfrom_timeout_ms, int stun_timeout_ms) {
    context->udp_is_connected = false;
    context->is_tcp = false;

    // Create UDP socket
    if ((context->socket = socketp_udp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    LOG_INFO("Connecting to server...");

    // Send Ack
    if (ack(context) < 0) {
        LOG_WARNING("Could not send ack to server %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    fractal_sleep(stun_timeout_ms);

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Connected to server on %s:%d! (Private %d)\n", inet_ntoa(context->addr.sin_addr),
             port, ntohs(context->addr.sin_port));

    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
}

int create_udp_client_context_stun(SocketContext* context, char* destination, int port,
                                   int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a UDP client context

        Arguments:
            context (SocketContext*): pointer to the context to populate
            destination (char*): the destination address
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection

        Returns:
            (int): -1 on failure, 0 on success
    */

    context->udp_is_connected = false;
    context->is_tcp = false;

    // Create UDP socket
    if ((context->socket = socketp_udp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;

    struct sockaddr_in stun_addr;
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_addr.s_addr = inet_addr(STUN_IP);
    stun_addr.sin_port = htons(STUN_PORT);

    StunRequest stun_request = {0};
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    LOG_INFO("Sending info request to STUN...");
    if (sendto(context->socket, (const char*)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr*)&stun_addr, sizeof(stun_addr)) < 0) {
        LOG_WARNING("Could not send message to STUN %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    StunEntry entry = {0};
    int recv_size;
    if ((recv_size = recvp(context, &entry, sizeof(entry))) < 0) {
        LOG_WARNING("Could not receive message from STUN %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
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

    LOG_INFO("Connecting to server...");

    // Open up the port
    if (sendp(context, NULL, 0) < 0) {
        LOG_ERROR("sendp(3) failed! Could not open up port! %d", get_last_network_error());
        return false;
    }
    fractal_sleep(150);

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Connected to server on %s:%d! (Private %d)\n", inet_ntoa(context->addr.sin_addr),
             port, ntohs(context->addr.sin_port));
    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
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

int recvp(SocketContext* context, void* buf, int len) {
    /*
        This will receive data over a socket

        Arguments:
            context (SocketContext*): The socket context to be used
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
int sendp(SocketContext* context, void* buf, int len) {
    /*
        This will send data over a socket

        Arguments:
            context (SocketContext*): The socket context to be used
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

int create_udp_context(SocketContext* context, char* destination, int port, int recvfrom_timeout_ms,
                       int stun_timeout_ms, bool using_stun, char* binary_aes_private_key) {
    /*
        Create a UDP context

        Arguments:
            context (SocketContext*): pointer to the context to populate
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

    if (destination == NULL) {
        // On the server, we create a network throttler to limit the
        // outgoing bitrate.
        context->network_throttler = network_throttler_create();
    } else {
        context->network_throttler = NULL;
    }

    if (using_stun) {
        if (destination == NULL)
            return create_udp_server_context_stun(context, port, recvfrom_timeout_ms,
                                                  stun_timeout_ms);
        else
            return create_udp_client_context_stun(context, destination, port, recvfrom_timeout_ms,
                                                  stun_timeout_ms);
    } else {
        if (destination == NULL)
            return create_udp_server_context(context, port, recvfrom_timeout_ms, stun_timeout_ms);
        else
            return create_udp_client_context(context, destination, port, recvfrom_timeout_ms,
                                             stun_timeout_ms);
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

int write_payload_to_packets(uint8_t* payload, size_t payload_size, int payload_id,
                             FractalPacketType packet_type, FractalPacket* packet_buffer,
                             size_t packet_buffer_length) {
    /*
        Split a payload into several packets approprately-sized
        for UDP transport, and write those files to a buffer.

        Arguments:
            payload (uint8_t*): The payload data to be split into packets
            payload_size (size_t): The size of the payload, in bytes
            payload_id (int): An ID for the UDP data (must be positive)
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
    size_t current_position = 0;

    // Calculate number of packets needed to send the payload, rounding up.
    int num_indices =
        (int)(payload_size / MAX_PAYLOAD_SIZE + (payload_size % MAX_PAYLOAD_SIZE == 0 ? 0 : 1));

    if ((size_t)num_indices > packet_buffer_length) {
        LOG_ERROR("Too many packets needed to send payload");
        return -1;
    }

    for (int packet_index = 0; packet_index < num_indices; ++packet_index) {
        FractalPacket* packet = &packet_buffer[packet_index];
        packet->type = packet_type;
        packet->payload_size = (int)min(payload_size - current_position, MAX_PAYLOAD_SIZE);
        packet->index = (short)packet_index;
        packet->id = payload_id;
        packet->num_indices = (short)num_indices;
        packet->is_a_nack = false;
        memcpy(packet->data, &payload[current_position], packet->payload_size);
        current_position += packet->payload_size;
    }

    return num_indices;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int send_tcp_packet_from_payload(SocketContext* context, FractalPacketType type, void* data,
                                 int len) {
    /*
        This will send a FractalPacket over TCP to the SocketContext context containing
        the specified payload. A FractalPacketType is also provided to describe the packet.

        Arguments:
            context (SocketContext*): The socket context
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

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int send_udp_packet(SocketContext* context, FractalPacket* packet, size_t packet_size) {
    /*
        This will send a FractalPacket over UDP to the SocketContext context. This
        function does not create the packet from raw data, but assumes that the
        packet has been prepared by the caller (e.g. fragmented into appropriately-sized)
        chunks by a fragmenter). This function assumes and checks that the packet is
        small enough to send without further breaking into smaller packets.

        Arguments:
            context (SocketContext*): The socket context
            packet (FractalPacket*): A pointer to the packet to be sent
            packet_size (size_t): The size of the packet to be sent

        Returns:
            (int): Will return -1 on failure, will return 0 on success
    */

    if (context == NULL) {
        LOG_WARNING("SocketContext is NULL");
        return -1;
    }

    // Use MAX_PACKET_SIZE here since we are checking the size of the packet itself.
    if (packet_size > MAX_PACKET_SIZE) {
        LOG_ERROR("Packet too large to send over UDP: %d", packet_size);
        return -1;
    }

    FractalPacket encrypted_packet;
    size_t encrypted_len = (size_t)encrypt_packet(packet, (int)packet_size, &encrypted_packet,
                                                  (unsigned char*)context->binary_aes_private_key);
    network_throttler_wait_byte_allocation(context->network_throttler, (size_t)encrypted_len);

#define RETRIES_ON_BUFFER_FULL 5
    for (int i = 0; i < RETRIES_ON_BUFFER_FULL; i++) {
        fractal_lock_mutex(context->mutex);
        int ret;
#if LOG_NETWORKING
        LOG_INFO("Sending a FractalPacket of size %d over UDP", packet_size);
#endif
        if (ENCRYPTING_PACKETS) {
            // Send encrypted during normal usage
            ret = sendp(context, &encrypted_packet, (int)encrypted_len);
        } else {
            // Send unencrypted during dev mode
            ret = sendp(context, packet, (int)packet_size);
        }
        fractal_unlock_mutex(context->mutex);
        if (ret < 0) {
            int error = get_last_network_error();
            LOG_WARNING("Unexpected UDP Packet Error: %d", error);
            if (error == 55) {
                fractal_sleep(100);
                continue;
            }
            return -1;
        }
    }

    return 0;
}
// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int send_udp_packet_from_payload(SocketContext* context, FractalPacketType type, void* data,
                                 int len, int id) {
    /*
        This will send a FractalPacket over UDP to the SocketContext context. A
        FractalPacketType is also provided to the receiving end. This function
        assumes and checks that the packet is small enough to send without breaking
        into smaller packets.

        Arguments:
            context (SocketContext*): The socket context
            type (FractalPacketType): The FractalPacketType, either VIDEO, AUDIO, or MESSAGE
            data (void*): A pointer to the data to be sent
            len (int): The number of bytes to send
            id (int): An ID for the UDP data

        Returns:
            (int): Will return -1 on failure, will return 0 on success
    */

    if (context == NULL) {
        LOG_WARNING("SocketContext is NULL");
        return -1;
    }

    // Use MAX_PAYLOAD_SIZE here since we are checking the size of the packet's
    // payload data.
    if ((size_t)len > MAX_PAYLOAD_SIZE) {
        LOG_ERROR("Payload too large to send over UDP: %d", len);
        return -1;
    }

    FractalPacket packet;
    packet.id = id;
    packet.type = type;
    packet.index = 0;
    packet.payload_size = len;
    packet.num_indices = 1;
    packet.is_a_nack = false;
    memcpy(packet.data, data, len);
    size_t packet_size = (size_t)get_packet_size(&packet);
    return send_udp_packet(context, &packet, packet_size);
}

int ack(SocketContext* context) {
    /*
        Send a 0-length packet over the socket. Used to keep-alive over NATs,
        and to check on the validity of the socket.

        Arguments:
            context (SocketContext*): The socket context

        Returns:
            (int): Will return -1 on failure, will return 0 on success.
                Failure implies that the socket is broken or the TCP
                connection has ended, use GetLastNetworkError() to learn
                more about the error
    */

    return sendp(context, NULL, 0);
}

FractalPacket* read_tcp_packet(SocketContext* context, bool should_recvp) {
    /*
        Receive a FractalPacket from a SocketContext, if any such packet exists

        Arguments:
            context (SocketContext*): The socket context
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

FractalPacket* read_udp_packet(SocketContext* context) {
    /*
        Receive a FractalPacket from a SocketContext, if any such packet exists

        Arguments:
            context (SocketContext*): The socket context

        Returns:
            (FractalPacket*): A pointer to the FractalPacket on success, NULL on failure
    */

    if (context == NULL) {
        LOG_WARNING("Context is NULL");
        return NULL;
    }

    // Wait to receive packet over UDP, until timing out
    FractalPacket encrypted_packet;
    int encrypted_len = recvp(context, &encrypted_packet, sizeof(encrypted_packet));

    static FractalPacket decrypted_packet;

    // If the packet was successfully received, then decrypt it
    if (encrypted_len > 0) {
        int decrypted_len;
        if (ENCRYPTING_PACKETS) {
            // Decrypt the packet
            decrypted_len = decrypt_packet(&encrypted_packet, encrypted_len, &decrypted_packet,
                                           (unsigned char*)context->binary_aes_private_key);
        } else {
            // The decrypted packet is just the original packet, during dev mode
            decrypted_len = encrypted_len;
            memcpy(&decrypted_packet, &encrypted_packet, encrypted_len);
        }
#if LOG_NETWORKING
        LOG_INFO("Received a FractalPacket of size %d over UDP", decrypted_len);
#endif

        // If there was an issue decrypting it, post warning and then
        // ignore the problem
        if (decrypted_len < 0) {
            if (encrypted_len == sizeof(StunEntry)) {
                StunEntry* e;
                e = (void*)&encrypted_packet;
                LOG_INFO("Maybe a map from public %d to private %d?", ntohs(e->private_port),
                         ntohs(e->private_port));
            }
            LOG_WARNING("Failed to decrypt packet");
            return NULL;
        }

        return &decrypted_packet;
    } else {
        if (encrypted_len < 0) {
            int error = get_last_network_error();
            switch (error) {
                case FRACTAL_ETIMEDOUT:
                    // LOG_ERROR("Read UDP Packet error: Timeout");
                case FRACTAL_EWOULDBLOCK:
                    // LOG_ERROR("Read UDP Packet error: Blocked");
                    // Break on expected network errors
                    break;
                default:
                    LOG_WARNING("Unexpected Packet Error: %d", error);
                    break;
            }
        } else {
            // Ignore packets of size 0
        }
        return NULL;
    }
}

void free_tcp_packet(FractalPacket* tcp_packet) {
    /*
        Frees a TCP packet created by read_tcp_packet

        Arguments:
            tcp_packet (FractalPacket*): The TCP packet to free
    */

    deallocate_region(tcp_packet);
}
