/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include <whist/core/whist.h>
#include <whist/network/network.h>
#include <whist/network/network_algorithm.h>
#include <whist/input/input.h>
#include <whist/logging/error_monitor.h>
#include <whist/utils/aes.h>
#include "whist/core/features.h"
#include <stdio.h>

#include "network.h"
#include "client.h"
#include "state.h"
#include "handle_client_message.h"

// UDP Waits for the first connection attempt,
#define UDP_CONNECTION_WAIT 5000
// After that we finish with TCP
#define TCP_CONNECTION_WAIT 1000

static int last_input_id = -1;

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Establishes UDP and TCP connection to client.
 *
 * @details                        If no UDP message is available, *wsmsg is
 *                                 set to NULL and *wsmsg_size is set to 0.
 *                                 Otherwise, *wsmsg is populated with a pointer
 *                                 to the next available UDP message and
 *                                 *wsmsg_size is set to the size of that
 *                                 message. The message need not be freed.
 *                                 Failure here
 *
 * @param state                    The global server state
 * @param client				   The target client
 * @param binary_aes_private_key   Key used to encrypt and decrypt communication
 *                                 with the client.
 *
 * @returns                        Returns -1 if either UDP or TCP connection
 *                                 fails or another error occurs, 0 on success.
 */
static int client_connect_socket(whist_server_state *state, Client *client,
                                 char *binary_aes_private_key);

/*
============================
Public Function Implementations
============================
*/

int broadcast_tcp_packet(Client *client, WhistPacketType type, void *data, int len) {
    if (send_packet(&client->tcp_context, type, (uint8_t *)data, len, -1, false) < 0) {
        LOG_WARNING("Failed to send TCP packet to client");
        return -1;
    }
    return 0;
}

int try_get_next_message_tcp(Client *client, WhistPacket **p_tcp_packet) {
    *p_tcp_packet = NULL;

    if (!socket_update(&client->tcp_context)) {
        // If TCP has disconnected, start deactivating the client
        start_deactivating_client(client);
    }

    WhistPacket *tcp_packet = get_packet(&client->tcp_context, PACKET_MESSAGE);
    if (tcp_packet) {
        LOG_INFO("Received TCP Packet: Size %d", tcp_packet->payload_size);
        *p_tcp_packet = tcp_packet;
    }
    return 0;
}

int try_get_next_message_udp(Client *client, WhistClientMessage *wcmsg, size_t *wcmsg_size) {
    *wcmsg_size = 0;

    memset(wcmsg, 0, sizeof(*wcmsg));

    if (!socket_update(&client->udp_context)) {
        // If UDP has disconnected, start deactivating the client
        start_deactivating_client(client);
    }

    WhistPacket *packet = get_packet(&client->udp_context, PACKET_MESSAGE);
    if (packet) {
        if (packet->payload_size < 0 || (int)sizeof(WhistClientMessage) < packet->payload_size) {
            LOG_INFO("Packet payload is out-of-bounds! %d instead of %d", packet->payload_size,
                     (int)sizeof(WhistClientMessage));
            free_packet(&client->udp_context, packet);
            return -1;
        }
        memcpy(wcmsg, packet->data, min(sizeof(*wcmsg), (size_t)packet->payload_size));
        if (packet->payload_size != get_wcmsg_size(wcmsg)) {
            LOG_WARNING("Packet is of the wrong size!: %d", packet->payload_size);
            LOG_WARNING("Type: %d", wcmsg->type);
            free_packet(&client->udp_context, packet);
            return -1;
        }
        *wcmsg_size = packet->payload_size;

        // Make sure that keyboard events are played in order
        if (wcmsg->type == MESSAGE_KEYBOARD || wcmsg->type == MESSAGE_KEYBOARD_STATE) {
            // Check that id is in order
            if (packet->id > last_input_id) {
                packet->id = last_input_id;
            } else {
                LOG_WARNING("Ignoring out of order keyboard input.");
                *wcmsg_size = 0;
                free_packet(&client->udp_context, packet);
                return 0;
            }
        }

        free_packet(&client->udp_context, packet);
    }
    return 0;
}

bool get_using_stun(void) {
    // decide whether the server is using stun.
    return false;
}

bool connect_client(void *state_raw) {
    whist_server_state *state = (whist_server_state *)state_raw;
    Client *client = state->client;

    FATAL_ASSERT(!client->is_active);

    state->connection_id = rand();
    client->connection_id = state->connection_id;

    // Client is not in use so we don't need to worry about anyone else
    // touching it
    if (client_connect_socket(state, state->client, state->config->binary_aes_private_key) != 0) {
        LOG_WARNING("Failed to establish connection with client.");
        return false;
    }

    LOG_INFO("Client successfully connected.");

    state->client_joined_after_window_name_broadcast = true;

    // We reset the input tracker when a new client connects
    reset_input(state->client_os);

    // Fill the network settings with default value, if we have a valid width and height
    if (state->client_width > 0 && state->client_height > 0 && state->client_dpi > 0) {
        udp_handle_network_settings(
            state->client->udp_context.context,
            get_default_network_settings(state->client_width, state->client_height,
                                         state->client_dpi));
    }

    return true;
}

/*
============================
Private Function Implementations
============================
*/

int client_connect_socket(whist_server_state *state, Client *client,
                          char *binary_aes_private_key_input) {
    if (!create_udp_socket_context(&client->udp_context, NULL, BASE_UDP_PORT, 1,
                                   UDP_CONNECTION_WAIT, false, binary_aes_private_key_input)) {
        LOG_WARNING("Failed UDP connection with client");
        return -1;
    }
    udp_register_nack_buffer(&client->udp_context, PACKET_VIDEO,
                             PACKET_HEADER_SIZE + LARGEST_VIDEOFRAME_SIZE, VIDEO_NACKBUFFER_SIZE);
    udp_register_nack_buffer(&client->udp_context, PACKET_AUDIO,
                             PACKET_HEADER_SIZE + LARGEST_AUDIOFRAME_SIZE, AUDIO_NACKBUFFER_SIZE);

    // NOTE: The server-side create_udp_socket_context call will finish immediately after the
    // handshake, But the UDP Client will be waiting until it gets a response. Thus, this TCP socket
    // will be open immediately (Before the TCP client tries to connect)
    // TODO #5539 (On Linux):
    //    The goal is, if we open the TCP server while the TCP client is trying to connect,
    //    the connection still succeeds. This is stronger, but unlike UDP, we can't
    //    easily control TCP's handshake. Can we compel TCP to send multiple SYN's?
    if (!create_tcp_socket_context(&client->tcp_context, NULL, BASE_TCP_PORT, 1,
                                   TCP_CONNECTION_WAIT, false, binary_aes_private_key_input)) {
        LOG_WARNING("Failed TCP connection with client");
        destroy_socket_context(&client->udp_context);
        return -1;
    }

    WhistTimer connection_timer;
    start_timer(&connection_timer);
    bool successful_handshake = false;

    while (socket_update(&client->tcp_context) &&
           get_timer(&connection_timer) < TCP_CONNECTION_WAIT) {
        WhistPacket *tcp_packet = get_packet(&client->tcp_context, PACKET_MESSAGE);

        // Check if we got an init message
        if (tcp_packet) {
            // Handle the init message
            WhistClientMessage *wcmsg = (WhistClientMessage *)tcp_packet->data;
            FATAL_ASSERT(wcmsg->type == CMESSAGE_INIT);
            handle_client_message(state, wcmsg);
            free_packet(&client->tcp_context, tcp_packet);

            // Reply to the init message
            WhistServerMessage reply = {0};
            reply.type = SMESSAGE_INIT_REPLY;
            whist_error_monitor_set_connection_id(state->connection_id);
            reply.init_reply.connection_id = state->connection_id;
            snprintf(reply.init_reply.git_revision, sizeof(reply.init_reply.git_revision), "%s",
                     whist_git_revision());
            reply.init_reply.feature_mask = whist_get_feature_mask();
            send_packet(&client->tcp_context, PACKET_MESSAGE, &reply, sizeof(reply), -1, false);

            // Done!
            successful_handshake = true;
            break;
        }
    }

    if (!successful_handshake) {
        LOG_INFO("Handshake not successful, destorying tcp and udp context!");
        destroy_socket_context(&client->tcp_context);
        destroy_socket_context(&client->udp_context);
    }

    return successful_handshake ? 0 : -1;
}
