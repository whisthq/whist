#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, need to
                                         // be at the very top
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "network.h"

static int reading_packet_len = 0;
static char reading_packet_buffer[LARGEST_TCP_PACKET];

static char decrypted_packet_buffer[LARGEST_TCP_PACKET];

int GetLastNetworkError() {
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

clock create_clock(int timeout_ms) {
    clock out;
#if defined(_WIN32)
    out = timeout_ms;
#else
    out.tv_sec = timeout_ms / 1000;
    out.tv_usec = (timeout_ms % 1000) * 1000;
#endif
    return out;
}

void set_timeout(SOCKET s, int timeout_ms) {
    // Sets the timeout for SOCKET s to be timeout_ms in milliseconds
    // Any recv calls will wait this long before timing out
    // -1 means that it will block indefinitely until a packet is received
    // 0 means that it will immediately return with whatever data is waiting in
    // the buffer
    if (timeout_ms < 0) {
        mprintf(
            "WARNING: This socket will blocking indefinitely. You will not be "
            "able to recover if a packet is never received\n");
        unsigned long mode = 0;

        FRACTAL_IOCTL_SCOKET(s, FIONBIO, &mode);

    } else if (timeout_ms == 0) {
        unsigned long mode = 1;
        FRACTAL_IOCTL_SCOKET(s, FIONBIO, &mode);
    } else {
        // Set to blocking when setting a timeout
        unsigned long mode = 0;
        FRACTAL_IOCTL_SCOKET(s, FIONBIO, &mode);

        clock read_timeout = create_clock(timeout_ms);

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&read_timeout,
                       sizeof(read_timeout)) < 0) {
            mprintf("Failed to set timeout\n");
            return;
        }
    }
}

void ClearReadingTCP() { reading_packet_len = 0; }

int recvp(struct SocketContext *context, void *buf, int len) {
    return recv(context->s, buf, len, 0);
}

int sendp(struct SocketContext *context, void *buf, int len) {
    return sendto(context->s, buf, len, 0, (struct sockaddr *)(&context->addr),
                  sizeof(context->addr));
}

bool tcp_connect(SOCKET s, struct sockaddr_in addr, int timeout_ms) {
    // Connect to TCP server
    set_timeout(s, 0);
    if (connect(s, (struct sockaddr *)(&addr), sizeof(addr)) < 0) {
        bool worked = GetLastNetworkError() == EINPROGRESS;

        if (!worked) {
            mprintf("Could not connect() over TCP to server %d\n",
                    GetLastNetworkError());
            closesocket(s);
            return false;
        }
    }
    // Select connection
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (select(s + 1, NULL, &set, NULL, &tv) <= 0) {
        mprintf("Could not select() over TCP to server %d\n",
                GetLastNetworkError());
        closesocket(s);
        return false;
    }

    set_timeout(s, timeout_ms);
    return true;
}

void *TryReadingTCPPacket(struct SocketContext *context) {
    if (!context->is_tcp) {
        mprintf("TryReadingTCPPacket received a context that is NOT TCP!\n");
        return NULL;
    }

    int len;

    do {
        // Try to fill up the buffer, in chunks of TCP_SEGMENT_SIZE, but don't
        // overflow LARGEST_TCP_PACKET
        len = recvp(
            context, reading_packet_buffer + reading_packet_len,
            min(TCP_SEGMENT_SIZE, LARGEST_TCP_PACKET - reading_packet_len));

        if (len < 0) {
            int err = GetLastNetworkError();
            if (err == ETIMEDOUT || err == EAGAIN) {
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
        int target_len = *((int *)reading_packet_buffer);

        // If the target len is valid, and actual len > target len, then we're
        // good to go
        if (target_len >= 0 && target_len <= LARGEST_TCP_PACKET &&
            actual_len >= target_len) {
            // Decrypt it
            int decrypted_len = decrypt_packet_n(
                (struct RTPPacket *)(reading_packet_buffer + sizeof(int)),
                target_len, (struct RTPPacket *)decrypted_packet_buffer,
                LARGEST_TCP_PACKET, (unsigned char *)PRIVATE_KEY);

            // Move the rest of the read bytes to the beginning of the buffer to
            // continue
            int start_next_bytes = sizeof(int) + target_len;
            for (unsigned long i = start_next_bytes;
                 i < sizeof(int) + actual_len; i++) {
                reading_packet_buffer[i - start_next_bytes] =
                    reading_packet_buffer[i];
            }
            reading_packet_len = actual_len - target_len;

            if (decrypted_len < 0) {
                mprintf("Could not decrypt TCP message\n");
                return NULL;
            } else {
                // Return the decrypted packet
                return decrypted_packet_buffer;
            }
        }
    }

    return NULL;
}

int CreateTCPServerContext(struct SocketContext *context, char *destination,
                           int port, int recvfrom_timeout_ms,
                           int stun_timeout_ms) {
    destination;  // TODO; remove useless parameter
    context->is_tcp = true;

    int opt;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);
    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        mprintf("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr),
             sizeof(origin_addr)) < 0) {
        mprintf("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    // Set listen queue
    set_timeout(context->s, stun_timeout_ms);
    if (listen(context->s, 3) < 0) {
        mprintf("Could not listen(2)! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    fd_set fd_read, fd_write;
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_SET(context->s, &fd_read);
    FD_SET(context->s, &fd_write);

    struct timeval tv;
    tv.tv_sec = stun_timeout_ms / 1000;
    tv.tv_usec = (stun_timeout_ms % 1000) * 1000;

    if (select(0, &fd_read, &fd_write, NULL, stun_timeout_ms > 0 ? &tv : NULL) <
        0) {
        mprintf("Could not select!\n");
        closesocket(context->s);
        return -1;
    }

    // Accept connection from client
    socklen_t slen = sizeof(context->addr);
    SOCKET new_socket;
    if ((new_socket = accept(context->s, (struct sockaddr *)(&context->addr),
                             &slen)) < 0) {
        mprintf("Did not receive response from client! %d\n",
                GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    closesocket(context->s);
    context->s = new_socket;

    mprintf("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
            ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateTCPServerContextStun(struct SocketContext *context, char *destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    destination;  // TODO; remove useless parameter
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
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    SOCKET udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sendto(udp_s, NULL, 0, 0, (struct sockaddr *)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);

    // Server connection protocol
    context->is_server = true;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        mprintf("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    struct sockaddr_in origin_addr;
    // Connect over TCP to STUN
    mprintf("Connecting to STUN TCP...\n");
    if (!tcp_connect(context->s, stun_addr, stun_timeout_ms)) {
        mprintf("Could not connect to STUN Server over TCP\n");
        return -1;
    }

    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->s, (struct sockaddr *)&origin_addr, &slen) < 0) {
        mprintf("Could not get sock name\n");
        closesocket(context->s);
        return -1;
    }

    // Send STUN request
    stun_request_t stun_request;
    stun_request.type = POST_INFO;
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        mprintf("Could not send STUN request to connected STUN server!\n");
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
                                      max(0, (int)sizeof(entry) - recv_size))) <
            0) {
            mprintf("Did not receive STUN response %d\n",
                    GetLastNetworkError());
            closesocket(context->s);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        mprintf("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->s);
        return -1;
    }

    // Print STUN response
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = entry.ip;
    client_addr.sin_port = entry.private_port;
    mprintf("TCP STUN notified of desired request from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    closesocket(context->s);

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }

    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        mprintf("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr),
             sizeof(origin_addr)) < 0) {
        mprintf("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    mprintf("WAIT\n");

    // Connect to client
    if (!tcp_connect(context->s, client_addr, stun_timeout_ms)) {
        mprintf("Could not connect to Client over TCP\n");
        return -1;
    }

    context->addr = client_addr;
    mprintf("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
            ntohs(context->addr.sin_port));
    set_timeout(context->s, recvfrom_timeout_ms);
    return 0;
}

int CreateTCPClientContext(struct SocketContext *context, char *destination,
                           int port, int recvfrom_timeout_ms,
                           int stun_timeout_ms) {
    stun_timeout_ms;  // TODO; remove useless parameter
    context->is_tcp = true;

    // Create TCP socket
    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    mprintf("Connecting to server...\n");

    SDL_Delay(200);

    // Connect to TCP server
    if (!tcp_connect(context->s, context->addr, stun_timeout_ms)) {
        mprintf("Could not connect to server over TCP\n");
        return -1;
    }

    mprintf("Connected on %s:%d!\n", destination, port);

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateTCPClientContextStun(struct SocketContext *context, char *destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
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
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Tell the STUN to use TCP
    SOCKET udp_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sendto(udp_s, NULL, 0, 0, (struct sockaddr *)&stun_addr, sizeof(stun_addr));
    closesocket(udp_s);
    // Client connection protocol
    context->is_server = false;

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        mprintf("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    // Connect to STUN server
    if (!tcp_connect(context->s, stun_addr, stun_timeout_ms)) {
        mprintf("Could not connect to STUN Server over TCP\n");
        return -1;
    }

    struct sockaddr_in origin_addr;
    socklen_t slen = sizeof(origin_addr);
    if (getsockname(context->s, (struct sockaddr *)&origin_addr, &slen) < 0) {
        mprintf("Could not get sock name\n");
        closesocket(context->s);
        return -1;
    }

    // Make STUN request
    stun_request_t stun_request;
    stun_request.type = ASK_INFO;
    stun_request.entry.ip = inet_addr(destination);
    stun_request.entry.public_port = htons((unsigned short)port);

    if (sendp(context, &stun_request, sizeof(stun_request)) < 0) {
        mprintf("Could not send STUN request to connected STUN server!\n");
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
                                      max(0, (int)sizeof(entry) - recv_size))) <
            0) {
            mprintf("Did not receive STUN response %d\n",
                    GetLastNetworkError());
            closesocket(context->s);
            return -1;
        }
        recv_size += single_recv_size;
    }

    if (recv_size != sizeof(entry)) {
        mprintf("TCP STUN Response packet of wrong size! %d\n", recv_size);
        closesocket(context->s);
        return -1;
    }

    // Print STUN response
    struct in_addr a;
    a.s_addr = entry.ip;
    mprintf("TCP STUN responded that the TCP server is located at %s:%d\n",
            inet_ntoa(a), ntohs(entry.private_port));

    closesocket(context->s);

    context->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }

    // Reuse addr
    opt = 1;
    if (setsockopt(context->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                   sizeof(opt)) < 0) {
        mprintf("Could not setsockopt SO_REUSEADDR\n");
        return -1;
    }

    // Bind to port
    if (bind(context->s, (struct sockaddr *)(&origin_addr),
             sizeof(origin_addr)) < 0) {
        mprintf("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = entry.ip;
    context->addr.sin_port = entry.private_port;

    mprintf("Connecting to server...\n");

    SDL_Delay(200);

    // Connect to TCP server
    if (!tcp_connect(context->s, context->addr, stun_timeout_ms)) {
        mprintf("Could not connect to server over TCP\n");
        return -1;
    }

    mprintf("Connected on %s:%d!\n", destination, port);

    set_timeout(context->s, recvfrom_timeout_ms);
    return 0;
}

int CreateTCPContext(struct SocketContext *context, char *origin,
                     char *destination, int port, int recvfrom_timeout_ms,
                     int stun_timeout_ms) {
#if USING_STUN
    if (origin[0] == 'C')
        return CreateTCPClientContextStun(context, destination, port,
                                          recvfrom_timeout_ms, stun_timeout_ms);
    else
        return CreateTCPServerContextStun(context, destination, port,
                                          recvfrom_timeout_ms, stun_timeout_ms);
#else
    if (origin[0] == 'C')
        return CreateTCPClientContext(context, destination, port,
                                      recvfrom_timeout_ms, stun_timeout_ms);
    else
        return CreateTCPServerContext(context, destination, port,
                                      recvfrom_timeout_ms, stun_timeout_ms);
#endif
}

int CreateUDPServerContext(struct SocketContext *context, char *destination,
                           int port, int recvfrom_timeout_ms,
                           int stun_timeout_ms) {
    destination;  // TODO; remove useless parameter
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

    // Bind the server port to the advertized public port
    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    if (bind(context->s, (struct sockaddr *)(&origin_addr),
             sizeof(origin_addr)) < 0) {
        mprintf("Failed to bind to port! %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    mprintf("Waiting for client to connect to %s:%d...\n", "localhost", port);

    socklen_t slen = sizeof(context->addr);
    stun_entry_t entry = {0};
    int recv_size;
    while ((recv_size = recvfrom(context->s, (char *)&entry, sizeof(entry), 0,
                                 (struct sockaddr *)(&context->addr), &slen)) <
           0) {
        mprintf("Did not receive response from client! %d\n",
                GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    set_timeout(context->s, 350);

    // Send acknowledgement of connection
    sendp(context, NULL, 0);

    mprintf("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
            ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPServerContextStun(struct SocketContext *context, char *destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    destination;  // TODO; remove useless parameter
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

    mprintf("Sending stun entry to STUN...\n");
    if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
        mprintf("Could not send message to STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    mprintf("Waiting for client to connect to %s:%d...\n", "localhost", port);

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
                                 (struct sockaddr *)(&context->addr), &slen)) <
           0) {
        // If we haven't spent too much time waiting, and our previous 100ms
        // poll failed, then send another STUN update
        if (GetTimer(recv_timer) * 1000 < stun_timeout_ms &&
            (GetLastNetworkError() == ETIMEDOUT ||
             GetLastNetworkError() == EAGAIN)) {
            if (sendto(context->s, (const char *)&stun_request,
                       sizeof(stun_request), 0, (struct sockaddr *)&stun_addr,
                       sizeof(stun_addr)) < 0) {
                mprintf("Could not send message to STUN %d\n",
                        GetLastNetworkError());
                closesocket(context->s);
                return -1;
            }
            continue;
        }
        mprintf("Did not receive response from client! %d\n",
                GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    set_timeout(context->s, 350);

    if (recv_size != sizeof(entry)) {
        mprintf("STUN response was not the size of an entry!\n");
        closesocket(context->s);
        return -1;
    }

    // Setup addr to open up port
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = entry.ip;
    context->addr.sin_port = entry.private_port;

    mprintf("Received STUN response, client connection desired from %s:%d\n",
            inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));

    // Open up port to receive message
    if (sendp(context, NULL, 0) < 0) {
        mprintf("Could not open up port!\n");
        closesocket(context->s);
        return -1;
    }

    SDL_Delay(150);

    // Send acknowledgement
    if (sendp(context, NULL, 0) < 0) {
        mprintf("Could not open up port!\n");
        closesocket(context->s);
        return -1;
    }

    // Wait for client to connect
    if (recvfrom(context->s, NULL, 0, 0, (struct sockaddr *)(&context->addr),
                 &slen) < 0) {
        mprintf("Did not receive client confirmation!\n");
        closesocket(context->s);
        return -1;
    }

    // Check that confirmation matches STUN's claimed client
    if (context->addr.sin_addr.s_addr != entry.ip ||
        context->addr.sin_port != entry.private_port) {
        mprintf(
            "Connection did not match STUN's claimed client, got %s:%d "
            "instead\n",
            inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));
        context->addr.sin_addr.s_addr = entry.ip;
        context->addr.sin_port = entry.private_port;
        mprintf("Should have been %s:%d!\n", inet_ntoa(context->addr.sin_addr),
                ntohs(context->addr.sin_port));
        closesocket(context->s);
        return -1;
    }

    mprintf("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
            ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPClientContext(struct SocketContext *context, char *destination,
                           int port, int recvfrom_timeout_ms,
                           int stun_timeout_ms) {
    context->is_tcp = false;

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
        return -1;
    }
    set_timeout(context->s, stun_timeout_ms);

    // Client connection protocol
    context->is_server = false;
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);

    mprintf("Connecting to server...\n");

    // Open up the port
    if (sendp(context, NULL, 0) < 0) {
        mprintf("Could not send message to server %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    SDL_Delay(150);

    // Send acknowledgement
    if (sendp(context, NULL, 0) < 0) {
        mprintf("Could not send message to server %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    // Receive server's acknowledgement of connection
    socklen_t slen = sizeof(context->addr);
    if (recvfrom(context->s, NULL, 0, 0, (struct sockaddr *)&context->addr,
                 &slen) < 0) {
        mprintf("Did not receive response from server! %d\n",
                GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    mprintf("Connected to server on %s:%d! (Private %d)\n",
            inet_ntoa(context->addr.sin_addr), port,
            ntohs(context->addr.sin_port));

    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPClientContextStun(struct SocketContext *context, char *destination,
                               int port, int recvfrom_timeout_ms,
                               int stun_timeout_ms) {
    context->is_tcp = false;

    // Create UDP socket
    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s <= 0) {  // Windows & Unix cases
        mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
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

    mprintf("Sending info request to STUN...\n");
    if (sendto(context->s, (const char *)&stun_request, sizeof(stun_request), 0,
               (struct sockaddr *)&stun_addr, sizeof(stun_addr)) < 0) {
        mprintf("Could not send message to STUN %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    stun_entry_t entry = {0};
    int recv_size;
    if ((recv_size = recvp(context, &entry, sizeof(entry))) < 0) {
        mprintf("Could not receive message from STUN %d\n",
                GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    if (recv_size != sizeof(entry)) {
        mprintf("STUN Response of incorrect size!\n");
        closesocket(context->s);
        return -1;
    } else if (entry.ip != stun_request.entry.ip ||
               entry.public_port != stun_request.entry.public_port) {
        mprintf("STUN Response IP and/or Public Port is incorrect!");
        closesocket(context->s);
        return -1;
    } else {
        mprintf("Received STUN response! Public %d is mapped to private %d\n",
                ntohs((unsigned short)entry.public_port),
                ntohs((unsigned short)entry.private_port));
        context->addr.sin_family = AF_INET;
        context->addr.sin_addr.s_addr = entry.ip;
        context->addr.sin_port = entry.private_port;
    }

    mprintf("Connecting to server...\n");

    // Open up the port
    if (sendp(context, NULL, 0) < 0) {
        mprintf("Could not send message to server %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    SDL_Delay(150);

    // Send acknowledgement
    if (sendp(context, NULL, 0) < 0) {
        mprintf("Could not send message to server %d\n", GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    // Receive server's acknowledgement of connection
    socklen_t slen = sizeof(context->addr);
    if (recvfrom(context->s, NULL, 0, 0, (struct sockaddr *)&context->addr,
                 &slen) < 0) {
        mprintf("Did not receive response from server! %d\n",
                GetLastNetworkError());
        closesocket(context->s);
        return -1;
    }

    mprintf("Connected to server on %s:%d! (Private %d)\n",
            inet_ntoa(context->addr.sin_addr), port,
            ntohs(context->addr.sin_port));
    set_timeout(context->s, recvfrom_timeout_ms);

    return 0;
}

int CreateUDPContext(struct SocketContext *context, char *origin,
                     char *destination, int port, int recvfrom_timeout_ms,
                     int stun_timeout_ms) {
#if USING_STUN
    if (origin[0] == 'C')
        return CreateUDPClientContextStun(context, destination, port,
                                          recvfrom_timeout_ms, stun_timeout_ms);
    else
        return CreateUDPServerContextStun(context, destination, port,
                                          recvfrom_timeout_ms, stun_timeout_ms);
#else
    if (origin[0] == 'C')
        return CreateUDPClientContext(context, destination, port,
                                      recvfrom_timeout_ms, stun_timeout_ms);
    else
        return CreateUDPServerContext(context, destination, port,
                                      recvfrom_timeout_ms, stun_timeout_ms);
#endif
}