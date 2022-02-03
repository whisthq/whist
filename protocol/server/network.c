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
#include <whist/input/input.h>
#include <whist/logging/error_monitor.h>
#include <whist/utils/aes.h>
#include <stdio.h>

#include "network.h"
#include "client.h"
#include "state.h"
#include "handle_client_message.h"

#define TCP_DISCOVERY_CONNECTION_WAIT 5000
#define UDP_CONNECTION_WAIT 1000
#define TCP_CONNECTION_WAIT 1000
#define BITS_IN_BYTE 8.0

int begin_time_to_exit = 60;

int last_input_id = -1;

/*
============================
Private Functions
============================
*/

int do_discovery_handshake(whist_server_state *state, SocketContext *context,
                           WhistClientMessage *wcmsg);

/*
============================
Private Function Implementations
============================
*/

static int handle_discovery_port_message(whist_server_state *state, SocketContext *context,
                                         bool *new_client) {
    /*
        Handle a message from the client over received over the discovery port.

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            new_client (bool*): pointer to indicate whether this message created a new client

        Returns:
            (int): 0 on success, -1 on failure
    */

    WhistPacket *tcp_packet;
    WhistTimer timer;
    start_timer(&timer);
    do {
        socket_update(context);
        tcp_packet = get_packet(context, PACKET_MESSAGE);
        whist_sleep(5);
    } while (tcp_packet == NULL && get_timer(&timer) < CLIENT_PING_TIMEOUT_SEC);
    // Exit on null tcp packet, otherwise analyze the resulting WhistClientMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive request over discovery port from client.");
        return -1;
    }

    WhistClientMessage *wcmsg = (WhistClientMessage *)tcp_packet->data;
    *new_client = false;

    switch (wcmsg->type) {
        case MESSAGE_DISCOVERY_REQUEST: {
            int user_id = wcmsg->discoveryRequest.user_id;

            if (!state->client.is_active) {
                state->client.user_id = user_id;

                if (do_discovery_handshake(state, context, wcmsg) != 0) {
                    LOG_WARNING("Discovery handshake failed.");
                }

                free_packet(context, tcp_packet);
                wcmsg = NULL;
                tcp_packet = NULL;

                *new_client = true;
            }
            break;
        }
        default: {
            return -1;
        }
    }

    return 0;
}

int do_discovery_handshake(whist_server_state *state, SocketContext *context,
                           WhistClientMessage *wcmsg) {
    /*
        Perform a discovery handshake over the discovery port socket context

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            wcmsg (WhistClientMessage*): discovery message sent from client

        Returns:
            (int): 0 on success, -1 on failure
    */

    handle_client_message(state, wcmsg);

    size_t wsmsg_size = sizeof(WhistServerMessage) + sizeof(WhistDiscoveryReplyMessage);

    WhistServerMessage *wsmsg = safe_malloc(wsmsg_size);
    memset(wsmsg, 0, sizeof(*wsmsg));
    wsmsg->type = MESSAGE_DISCOVERY_REPLY;

    WhistDiscoveryReplyMessage *reply_msg = (WhistDiscoveryReplyMessage *)wsmsg->discovery_reply;

    reply_msg->udp_port = state->client.udp_port;
    reply_msg->tcp_port = state->client.tcp_port;

    // Set connection ID in error monitor.
    whist_error_monitor_set_connection_id(state->connection_id);

    // Send connection ID to client
    reply_msg->connection_id = state->connection_id;

    LOG_INFO("Sending discovery packet");
    LOG_INFO("wsmsg size is %d", (int)wsmsg_size);
    if (send_packet(context, PACKET_MESSAGE, (uint8_t *)wsmsg, (int)wsmsg_size, -1, false) < 0) {
        LOG_ERROR("Failed to send discovery reply message.");
        free(wsmsg);
        return -1;
    }

    free(wsmsg);

    LOG_INFO("Discovery handshake succeeded.");
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

int connect_client(Client *client, bool using_stun, char *binary_aes_private_key_input) {
    if (!create_udp_socket_context(&client->udp_context, NULL, client->udp_port, 1,
                                   UDP_CONNECTION_WAIT, using_stun, binary_aes_private_key_input)) {
        LOG_ERROR("Failed UDP connection with client");
        return -1;
    }
    udp_register_nack_buffer(&client->udp_context, PACKET_VIDEO,
                             PACKET_HEADER_SIZE + LARGEST_VIDEOFRAME_SIZE, VIDEO_NACKBUFFER_SIZE);
    udp_register_nack_buffer(&client->udp_context, PACKET_AUDIO,
                             PACKET_HEADER_SIZE + LARGEST_AUDIOFRAME_SIZE, AUDIO_NACKBUFFER_SIZE);

    if (!create_tcp_socket_context(&client->tcp_context, NULL, client->tcp_port, 1,
                                   TCP_CONNECTION_WAIT, using_stun, binary_aes_private_key_input)) {
        LOG_WARNING("Failed TCP connection with client");
        destroy_socket_context(&client->udp_context);
        return -1;
    }
    return 0;
}

int disconnect_client(Client *client) {
    destroy_socket_context(&client->udp_context);
    destroy_socket_context(&client->tcp_context);
    return 0;
}

int broadcast_udp_packet(Client *client, WhistPacketType type, void *data, int len, int packet_id) {
    if (packet_id <= 0) {
        LOG_WARNING("Packet IDs must be positive!");
        return -1;
    }

    if (client->is_active) {
        if (send_packet(&(client->udp_context), type, data, len, packet_id, false) < 0) {
            LOG_WARNING("Failed to send UDP packet to client");
            return -1;
        }
    }
    return 0;
}

int broadcast_tcp_packet(Client *client, WhistPacketType type, void *data, int len) {
    if (client->is_active) {
        read_lock(&client->tcp_rwlock);
        if (send_packet(&(client->tcp_context), type, (uint8_t *)data, len, -1, false) < 0) {
            LOG_WARNING("Failed to send TCP packet to client");
            return -1;
        }
        read_unlock(&client->tcp_rwlock);
    }
    return 0;
}

int try_get_next_message_tcp(Client *client, WhistPacket **p_tcp_packet) {
    *p_tcp_packet = NULL;

    if (!socket_update(&client->tcp_context)) {
        LOG_INFO("TCP socket connection lost!");
        if (!client->is_deactivating) {
            if (reap_timed_out_client(client, CLIENT_PING_TIMEOUT_SEC) != 0) {
                LOG_ERROR("Failed to reap timed out clients.");
            }
        }
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

    socket_update(&client->udp_context);
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

int multithreaded_manage_client(void *opaque) {
    whist_server_state *state = (whist_server_state *)opaque;
    whist_server_config *config = state->config;

    SocketContext discovery_context;
    bool new_client;

    WhistTimer last_update_timer;
    start_timer(&last_update_timer);

    state->connection_id = rand();

    bool first_client_connected = false;  // set to true once the first client has connected
    bool disable_timeout = false;
    if (config->begin_time_to_exit == -1) {
        // client has `begin_time_to_exit` seconds to connect when the server first goes up.
        // If the variable is -1, disable auto-exit.
        disable_timeout = true;
    }
    WhistTimer first_client_timer;
    // start this now and then discard when first client has connected
    start_timer(&first_client_timer);

    while (!state->exiting) {
        LOG_INFO("Is a client connected? %s", state->client.is_active ? "yes" : "no");

        // If all threads have stopped using the active client, we can finally quit it
        if (state->client.is_deactivating && !threads_still_holding_active()) {
            if (quit_client(&state->client) != 0) {
                LOG_ERROR("Failed to quit client.");
            } else {
                state->client.is_deactivating = false;
                LOG_INFO("Successfully quit client.");
            }
        }

        if (!state->client.is_active) {
            state->connection_id = rand();

            // container exit logic -
            //  * client has connected before but now none are connected
            //  * client has not connected in `begin_time_to_exit` secs of server being up
            // We don't place this in a lock because:
            //  * if the first client connects right on the threshold of begin_time_to_exit, it
            //  doesn't matter if we disconnect
            if (!disable_timeout && (first_client_connected || (get_timer(&first_client_timer) >
                                                                config->begin_time_to_exit))) {
                state->exiting = true;
            }
        }
        // Even without multiclient, we need this for TCP recovery over the discovery port
        if (!create_tcp_socket_context(&discovery_context, NULL, PORT_DISCOVERY, 1,
                                       TCP_DISCOVERY_CONNECTION_WAIT, get_using_stun(),
                                       config->binary_aes_private_key)) {
            continue;
        }

        // This can either be a new client connecting, or an existing client asking for a TCP
        //     connection to be recovered. We use the discovery port because it is always
        //     accepting connections and is reliable for both discovery and recovery messages.
        if (handle_discovery_port_message(state, &discovery_context, &new_client) != 0) {
            LOG_WARNING("Discovery port message could not be handled.");
            destroy_socket_context(&discovery_context);
            continue;
        }

        // Destroy the ephemeral discovery context
        destroy_socket_context(&discovery_context);

        // If the handled message was not for a discovery handshake, then skip
        //     over everything that is necessary for setting up a new client
        if (!new_client) {
            continue;
        }

        // Client is not in use so we don't need to worry about anyone else
        // touching it
        if (connect_client(&state->client, get_using_stun(), config->binary_aes_private_key) != 0) {
            LOG_WARNING("Failed to establish connection with client.");
            continue;
        }

        LOG_INFO("Client connected.");

        if (!state->client.is_active) {
            // we have went from 0 clients to 1 client, so we have got our first client
            // this variable should never be set back to false after this
            first_client_connected = true;
        }
        state->client_joined_after_window_name_broadcast = true;

        // We reset the input tracker when a new client connects
        reset_input(state->client_os);

        // When a new client has been connected, we want all threads to hold client active again
        reset_threads_holding_active_count(&state->client);
        state->client.is_active = true;
    }

    return 0;
}
