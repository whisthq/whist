/**
 * Copyright Fractal Computers, Inc. 2021
 * @file network.c
 * @brief This file contains server-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

Use these functions for any server-specific networking needs.
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>
#include <fractal/network/network.h>
#include <fractal/input/input.h>
#include <fractal/utils/error_monitor.h>
#include <fractal/utils/aes.h>
#include <stdio.h>

#include "network.h"
#include "client.h"
#include "handle_client_message.h"

#define UDP_CONNECTION_WAIT 1000
#define TCP_CONNECTION_WAIT 1000
#define BITS_IN_BYTE 8.0

extern Client clients[MAX_NUM_CLIENTS];
extern char binary_aes_private_key[16];
extern volatile int connection_id;
extern volatile bool exiting;
extern int sample_rate;
extern bool client_joined_after_window_name_broadcast;
int begin_time_to_exit = 60;

int last_input_id = -1;

/*
============================
Private Functions
============================
*/

int do_discovery_handshake(SocketContext *context, int *client_id);

/*
============================
Private Function Implementations
============================
*/

int do_discovery_handshake(SocketContext *context, int *client_id) {
    FractalPacket *tcp_packet;
    clock timer;
    start_timer(&timer);
    do {
        tcp_packet = read_tcp_packet(context, true);
        fractal_sleep(5);
    } while (tcp_packet == NULL && get_timer(timer) < CLIENT_PING_TIMEOUT_SEC);
    // Exit on null tcp packet, otherwise analyze the resulting FractalClientMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive discovery request from client.");
        closesocket(context->socket);
        return -1;
    }

    FractalClientMessage *fcmsg = (FractalClientMessage *)tcp_packet->data;
    int user_id = fcmsg->discoveryRequest.user_id;

    read_lock(&is_active_rwlock);
    bool found;
    int ret;
    if ((ret = try_find_client_id_by_user_id(user_id, &found, client_id)) != 0) {
        LOG_ERROR(
            "Failed to try to find client ID by user ID. "
            " (User ID: %s)",
            user_id);
    }
    if (ret == 0 && found) {
        read_unlock(&is_active_rwlock);
        write_lock(&is_active_rwlock);
        ret = quit_client(*client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to quit client. (ID: %d)", *client_id);
        }
        write_unlock(&is_active_rwlock);
    } else {
        ret = get_available_client_id(client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to find available client ID.");
            closesocket(context->socket);
        }
        read_unlock(&is_active_rwlock);
        if (ret != 0) {
            free_tcp_packet(tcp_packet);
            return -1;
        }
    }

    clients[*client_id].user_id = user_id;
    LOG_INFO("Found ID for client. (ID: %d)", *client_id);

    // TODO: Should check for is_controlling, but that happens after this function call
    handle_client_message(fcmsg, *client_id, true);
    // fcmsg points into tcp_packet, but after this point, we don't use either,
    // so here we free the tcp packet
    free_tcp_packet(tcp_packet);
    fcmsg = NULL;
    tcp_packet = NULL;

    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage *fsmsg = safe_malloc(fsmsg_size);
    memset(fsmsg, 0, sizeof(*fsmsg));
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage *reply_msg =
        (FractalDiscoveryReplyMessage *)fsmsg->discovery_reply;

    reply_msg->client_id = *client_id;
    reply_msg->UDP_port = clients[*client_id].UDP_port;
    reply_msg->TCP_port = clients[*client_id].TCP_port;

    // Set connection ID in error monitor.
    error_monitor_set_connection_id(connection_id);

    // Send connection ID to client
    reply_msg->connection_id = connection_id;
    reply_msg->audio_sample_rate = sample_rate;

    LOG_INFO("Sending discovery packet");
    LOG_INFO("Fsmsg size is %d", (int)fsmsg_size);
    if (send_tcp_packet(context, PACKET_MESSAGE, (uint8_t *)fsmsg, (int)fsmsg_size) < 0) {
        LOG_ERROR("Failed to send send discovery reply message.");
        closesocket(context->socket);
        free(fsmsg);
        return -1;
    }

    closesocket(context->socket);
    free(fsmsg);
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

int connect_client(int id, bool using_stun, char *binary_aes_private_key_input) {
    if (create_udp_context(&(clients[id].UDP_context), NULL, clients[id].UDP_port, 1,
                           UDP_CONNECTION_WAIT, using_stun, binary_aes_private_key_input) < 0) {
        LOG_ERROR("Failed UDP connection with client (ID: %d)", id);
        return -1;
    }

    if (create_tcp_context(&(clients[id].TCP_context), NULL, clients[id].TCP_port, 1,
                           TCP_CONNECTION_WAIT, using_stun, binary_aes_private_key_input) < 0) {
        LOG_WARNING("Failed TCP connection with client (ID: %d)", id);
        closesocket(clients[id].UDP_context.socket);
        return -1;
    }
    return 0;
}

int disconnect_client(int id) {
    closesocket(clients[id].UDP_context.socket);
    closesocket(clients[id].TCP_context.socket);
    return 0;
}

int disconnect_clients(void) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (disconnect_client(id) != 0) {
                LOG_ERROR("Failed to disconnect client (ID: %d)", id);
                ret = -1;
            } else {
                clients[id].is_active = false;
            }
        }
    }
    return ret;
}

int broadcast_ack(void) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            ack(&(clients[id].TCP_context));
            ack(&(clients[id].UDP_context));
        }
    }
    return ret;
}

int broadcast_udp_packet(FractalPacketType type, void *data, int len, int id, int burst_bitrate,
                         FractalPacket *packet_buffer, int *packet_len_buffer) {
    if (id <= 0) {
        LOG_WARNING("IDs must be positive!");
        return -1;
    }

    int payload_size;
    int curr_index = 0, i = 0;

    int num_indices = len / MAX_PAYLOAD_SIZE + (len % MAX_PAYLOAD_SIZE == 0 ? 0 : 1);

    double max_bytes_per_second = burst_bitrate / BITS_IN_BYTE;

    clock packet_timer;
    start_timer(&packet_timer);

    while (curr_index < len) {
        // Delay distribution of packets as needed
        while (burst_bitrate > 0 &&
               curr_index - 5000 > get_timer(packet_timer) * max_bytes_per_second) {
            fractal_sleep(1);
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

        // Send it off
        for (int j = 0; j < MAX_NUM_CLIENTS; j++) {
            if (clients[j].is_active) {
                // Encrypt the packet with AES
                FractalPacket encrypted_packet;
                int encrypt_len =
                    encrypt_packet(packet, packet_size, &encrypted_packet,
                                   (unsigned char *)clients[j].UDP_context.binary_aes_private_key);

                fractal_lock_mutex(clients[j].UDP_context.mutex);
                int sent_size = sendp(&(clients[j].UDP_context), &encrypted_packet, encrypt_len);
                fractal_unlock_mutex(clients[j].UDP_context.mutex);
                if (sent_size < 0) {
                    int error = get_last_network_error();
                    LOG_INFO("Unexpected Packet Error: %d", error);
                    LOG_WARNING("Failed to send UDP packet to client id: %d", j);
                }
            }
        }

        i++;
        curr_index += payload_size;
    }

    // LOG_INFO( "Packet Time: %f", get_timer( packet_timer ) );

    return 0;
}

int broadcast_tcp_packet(FractalPacketType type, void *data, int len) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (send_tcp_packet(&(clients[id].TCP_context), type, (uint8_t *)data, len) < 0) {
                LOG_WARNING("Failed to send TCP packet to client id: %d", id);
                if (ret == 0) ret = -1;
            }
        }
    }
    return ret;
}

clock last_tcp_read;
bool has_read = false;
int try_get_next_message_tcp(int client_id, FractalPacket **p_tcp_packet) {
    *p_tcp_packet = NULL;

    // Check if 20ms has passed since last TCP recvp, since each TCP recvp read takes 8ms
    bool should_recvp = false;
    if (!has_read || get_timer(last_tcp_read) * 1000.0 > 20.0) {
        should_recvp = true;
        start_timer(&last_tcp_read);
        has_read = true;
    }

    FractalPacket *tcp_packet = read_tcp_packet(&(clients[client_id].TCP_context), should_recvp);
    if (tcp_packet) {
        LOG_INFO("Received TCP Packet (Probably clipboard): Size %d", tcp_packet->payload_size);
        *p_tcp_packet = tcp_packet;
    }
    return 0;
}

int try_get_next_message_udp(int client_id, FractalClientMessage *fcmsg, size_t *fcmsg_size) {
    *fcmsg_size = 0;

    memset(fcmsg, 0, sizeof(*fcmsg));

    FractalPacket *packet = read_udp_packet(&(clients[client_id].UDP_context));
    if (packet) {
        memcpy(fcmsg, packet->data, max(sizeof(*fcmsg), (size_t)packet->payload_size));
        if (packet->payload_size != get_fmsg_size(fcmsg)) {
            LOG_WARNING("Packet is of the wrong size!: %d", packet->payload_size);
            LOG_WARNING("Type: %d", fcmsg->type);
            return -1;
        }
        *fcmsg_size = packet->payload_size;

        // Make sure that keyboard events are played in order
        if (fcmsg->type == MESSAGE_KEYBOARD || fcmsg->type == MESSAGE_KEYBOARD_STATE) {
            // Check that id is in order
            if (packet->id > last_input_id) {
                packet->id = last_input_id;
            } else {
                LOG_WARNING("Ignoring out of order keyboard input.");
                *fcmsg_size = 0;
                return 0;
            }
        }
    }
    return 0;
}

// TODO: Figure out if this value needs this redefinition to work well, or what single value this
// should be
#undef TCP_CONNECTION_WAIT
#define TCP_CONNECTION_WAIT 5000

bool get_using_stun() {
    // decide whether the server is using stun.
    return false;
}

int multithreaded_manage_clients(void *opaque) {
    UNUSED(opaque);

    SocketContext discovery_context;
    int client_id;

    clock last_update_timer;
    start_timer(&last_update_timer);

    connection_id = rand();

    bool first_client_connected = false;      // set to true once the first client has connected
    bool disable_timeout = false;
    if (begin_time_to_exit ==
        -1) {  // client has `begin_time_to_exit` seconds to connect when the server first goes up.
               // If the variable is -1, disable auto-exit.
        disable_timeout = true;
    }
    clock first_client_timer;  // start this now and then discard when first client has connected
    start_timer(&first_client_timer);

    while (!exiting) {
        read_lock(&is_active_rwlock);

        int saved_num_active_clients = num_active_clients;

        read_unlock(&is_active_rwlock);

        LOG_INFO("Num Active Clients %d", saved_num_active_clients);

        if (saved_num_active_clients == 0) {
            connection_id = rand();

            // container exit logic -
            //  * clients have connected before but now none are connected
            //  * no clients have connected in `begin_time_to_exit` secs of server being up
            // We don't place this in a lock because:
            //  * if the first client connects right on the threshold of begin_time_to_exit, it
            //  doesn't matter if we disconnect
            if (!disable_timeout &&
                (first_client_connected || (get_timer(first_client_timer) > begin_time_to_exit))) {
                exiting = true;
            }
        }

        if (create_tcp_context(&discovery_context, NULL, PORT_DISCOVERY, 1, TCP_CONNECTION_WAIT,
                               get_using_stun(), binary_aes_private_key) < 0) {
            continue;
        }

        if (do_discovery_handshake(&discovery_context, &client_id) != 0) {
            LOG_WARNING("Discovery handshake failed.");
            continue;
        }

        LOG_INFO("Discovery handshake succeeded. (ID: %d)", client_id);

        // Client is not in use so we don't need to worry about anyone else
        // touching it
        if (connect_client(client_id, get_using_stun(), binary_aes_private_key) != 0) {
            LOG_WARNING(
                "Failed to establish connection with client. "
                "(ID: %d)",
                client_id);
            continue;
        }

        write_lock(&is_active_rwlock);

        LOG_INFO("Client connected. (ID: %d)", client_id);

        // We probably need to lock these. Argh.
        if (host_id == -1) {
            host_id = client_id;
        }

        if (num_active_clients == 0) {
            // we have went from 0 clients to 1 client, so we have got our first client
            // this variable should never be set back to false after this
            first_client_connected = true;
        }
        num_active_clients++;
        client_joined_after_window_name_broadcast = true;
        /* Make everyone a controller */
        clients[client_id].is_controlling = true;
        num_controlling_clients++;
        // if (num_controlling_clients == 0) {
        //     clients[client_id].is_controlling = true;
        //     num_controlling_clients++;
        // }

        if (clients[client_id].is_controlling) {
            // Reset input system when a new input controller arrives
            reset_input();
        }

        start_timer(&(clients[client_id].last_ping));

        clients[client_id].is_active = true;

        write_unlock(&is_active_rwlock);
    }

    return 0;
}
