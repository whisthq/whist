#include "udp.h"
#include <fractal/utils/aes.h>

// Define how many times to retry sending a UDP packet in case of Error 55 (buffer full). The
// current value (5) is an arbitrary choice that was found to work well in practice.
#define RETRIES_ON_BUFFER_FULL 5

extern unsigned short port_mappings[USHRT_MAX + 1];

/*
============================
UDP Implementation of Network.h Interface
============================
*/

int udp_ack(void* raw_context) {
    SocketContextData* context = raw_context;
    return sendp(context, NULL, 0);
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int udp_send_packet(void* raw_context, FractalPacket* packet, size_t packet_size) {
    /*
        This will send a FractalPacket over UDP to the SocketContextData context. This
        function does not create the packet from raw data, but assumes that the
        packet has been prepared by the caller (e.g. fragmented into appropriately-sized)
        chunks by a fragmenter). This function assumes and checks that the packet is
        small enough to send without further breaking into smaller packets.

        Arguments:
            context (SocketContextData*): The socket context
            packet (FractalPacket*): A pointer to the packet to be sent
            packet_size (size_t): The size of the packet to be sent

        Returns:
            (int): Will return -1 on failure, will return 0 on success
    */

    SocketContextData* context = raw_context;
    if (context == NULL) {
        LOG_ERROR("SocketContextData is NULL");
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

    // If sending fails because of no buffer space available on the system, retry a few times.
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
            if (error == ENOBUFS) {
                LOG_WARNING("Unexpected UDP Packet Error: %d, retrying to send packet in 5ms!",
                            error);
                fractal_sleep(5);
                continue;
            } else {
                LOG_WARNING("Unexpected UDP Packet Error: %d", error);
                return -1;
            }
        } else {
            break;
        }
    }

    return 0;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int udp_send_packet_from_payload(void* raw_context, FractalPacketType type, void* data, int len,
                                 int id) {
    /*
        This will send a FractalPacket over UDP to the SocketContextData context. A
        FractalPacketType is also provided to the receiving end. This function
        assumes and checks that the packet is small enough to send without breaking
        into smaller packets.

        Arguments:
            context (SocketContextData*): The socket context
            type (FractalPacketType): The FractalPacketType, either VIDEO, AUDIO, or MESSAGE
            data (void*): A pointer to the data to be sent
            len (int): The number of bytes to send
            id (int): An ID for the UDP data

        Returns:
            (int): Will return -1 on failure, will return 0 on success
    */

    SocketContextData* context = raw_context;
    if (context == NULL) {
        LOG_ERROR("SocketContextData is NULL");
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
    return udp_send_packet(context, &packet, packet_size);
}

FractalPacket* udp_read_packet(void* raw_context, bool should_recv) {
    /*
        Receive a FractalPacket from a SocketContextData, if any such packet exists

        Arguments:
            context (SocketContextData*): The socket context

        Returns:
            (FractalPacket*): A pointer to the FractalPacket on success, NULL on failure
    */

    SocketContextData* context = raw_context;

    if (should_recv == false) {
        LOG_ERROR("should_recv should only be false in TCP contexts");
        return NULL;
    }

    if (context->decrypted_packet_used) {
        LOG_ERROR("Cannot use context->decrypted_packet buffer! Still being used somewhere else!");
        return NULL;
    }

    // Wait to receive packet over TCP, until timing out
    FractalPacket encrypted_packet;
    int encrypted_len = recv(context->socket, &encrypted_packet, sizeof(encrypted_packet), 0);

    // If the packet was successfully received, then decrypt it
    if (encrypted_len > 0) {
        int decrypted_len;
        if (ENCRYPTING_PACKETS) {
            // Decrypt the packet
            decrypted_len =
                decrypt_packet(&encrypted_packet, encrypted_len, &context->decrypted_packet,
                               (unsigned char*)context->binary_aes_private_key);
        } else {
            // The decrypted packet is just the original packet, during dev mode
            decrypted_len = encrypted_len;
            memcpy(&context->decrypted_packet, &encrypted_packet, encrypted_len);
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

        context->decrypted_packet_used = true;
        return &context->decrypted_packet;
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

void udp_free_packet(void* raw_context, FractalPacket* udp_packet) {
    /*
        Frees a UDP packet created by udp_read_packet

        Arguments:
            tcp_packet (FractalPacket*): The udp packet to free

        TODO (abecohen): Change udp_read_packet to use malloc
        and then add "deallocate_region(udp_packet);" to this function.
    */

    SocketContextData* context = raw_context;

    if (!context->decrypted_packet_used) {
        LOG_ERROR("Called udp_free_packet, but there was no udp_packet to free!");
        return;
    }
    if (udp_packet != &context->decrypted_packet) {
        LOG_ERROR("The wrong pointer was passed into udp_free_packet!");
    }

    // Free the one buffer
    context->decrypted_packet_used = false;
}

int udp_write_payload_to_packets(uint8_t* payload, size_t payload_size, int payload_id,
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

void udp_destroy_socket_context(void* raw_context) {
    SocketContextData* context = raw_context;

    closesocket(context->socket);
    if (context->network_throttler != NULL) {
        network_throttler_destroy(context->network_throttler);
    }
    fractal_destroy_mutex(context->mutex);
    free(context);
}

/*
============================
Private Function Implementations
============================
*/

int create_udp_server_context(void* raw_context, int port, int recvfrom_timeout_ms,
                              int stun_timeout_ms) {
    /*
        Create a UDP server context

        Arguments:
            context (SocketContextData*): pointer to the context to populate
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            stun_timeout_ms (int): timeout, in milliseconds, for socket connection

        Returns:
            (int): -1 on failure, 0 on success
    */

    SocketContextData* context = raw_context;

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

int create_udp_server_context_stun(SocketContextData* context, int port, int recvfrom_timeout_ms,
                                   int stun_timeout_ms) {
    /*
        Create a UDP client context over STUN server

        Arguments:
            context (SocketContextData*): pointer to the context to populate
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

int create_udp_client_context(SocketContextData* context, char* destination, int port,
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
    if (udp_ack(context) < 0) {
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

int create_udp_client_context_stun(SocketContextData* context, char* destination, int port,
                                   int recvfrom_timeout_ms, int stun_timeout_ms) {
    /*
        Create a UDP client context

        Arguments:
            context (SocketContextData*): pointer to the context to populate
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
    if ((recv_size = recv(context->socket, &entry, sizeof(entry), 0)) < 0) {
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

/*
============================
Public Function Implementations
============================
*/

bool create_udp_socket_context(SocketContext* network_context, char* destination, int port,
                               int recvfrom_timeout_ms, int connection_timeout_ms, bool using_stun,
                               char* binary_aes_private_key) {
    // Populate function pointer table
    network_context->ack = udp_ack;
    network_context->read_packet = udp_read_packet;
    network_context->send_packet = udp_send_packet;
    network_context->send_packet_from_payload = udp_send_packet_from_payload;
    network_context->free_packet = udp_free_packet;
    network_context->write_payload_to_packets = udp_write_payload_to_packets;
    network_context->destroy_socket_context = udp_destroy_socket_context;

    // Create the SocketContextData, and set to zero
    SocketContextData* context = safe_malloc(sizeof(SocketContextData));
    memset(context, 0, sizeof(SocketContextData));
    network_context->context = context;

    // Map Port
    if ((int)((unsigned short)port) != port) {
        LOG_ERROR("Port invalid: %d", port);
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

    int ret;
    if (using_stun) {
        if (destination == NULL)
            ret = create_udp_server_context_stun(context, port, recvfrom_timeout_ms,
                                                 connection_timeout_ms);
        else
            ret = create_udp_client_context_stun(context, destination, port, recvfrom_timeout_ms,
                                                 connection_timeout_ms);
    } else {
        if (destination == NULL)
            ret = create_udp_server_context(context, port, recvfrom_timeout_ms,
                                            connection_timeout_ms);
        else
            ret = create_udp_client_context(context, destination, port, recvfrom_timeout_ms,
                                            connection_timeout_ms);
    }

    if (ret == 0) {
        // socket options = TCP_NODELAY IPTOS_LOWDELAY SO_RCVBUF=65536
        // Windows Socket 65535 Socket options apply to all sockets.
        // this is set to stop the kernel from buffering too much, thereby
        // getting the data to us faster for lower latency
        int a = 65535;
        if (setsockopt(context->socket, SOL_SOCKET, SO_RCVBUF, (const char*)&a, sizeof(int)) ==
            -1) {
            LOG_ERROR("Error setting socket opts: %d", get_last_network_error());
            closesocket(context->socket);
            return false;
        }
        return true;
    } else {
        return false;
    }
}
