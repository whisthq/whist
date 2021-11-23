/**
 * Copyright 2021 Whist Technologies, Inc.
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
#include <fractal/logging/error_monitor.h>
#include <fractal/utils/aes.h>
#include <stdio.h>

#include "network.h"
#include "client.h"
#include "main.h"
#include "handle_client_message.h"

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
                           FractalClientMessage *fcmsg);

/*
============================
Private Function Implementations
============================
*/

int handle_discovery_port_message(whist_server_state *state, SocketContext *context,
                                  bool *new_client) {
    /*
        Handle a message from the client over received over the discovery port.

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            new_client (bool*): pointer to indicate whether this message created a new client

        Returns:
            (int): 0 on success, -1 on failure
    */

    FractalPacket *tcp_packet;
    clock timer;
    start_timer(&timer);
    do {
        tcp_packet = read_packet(context, true);
        fractal_sleep(5);
    } while (tcp_packet == NULL && get_timer(timer) < CLIENT_PING_TIMEOUT_SEC);
    // Exit on null tcp packet, otherwise analyze the resulting FractalClientMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive request over discovery port from client.");
        destroy_socket_context(context);
        return -1;
    }

    FractalClientMessage *fcmsg = (FractalClientMessage *)tcp_packet->data;
    *new_client = false;

    switch (fcmsg->type) {
        case MESSAGE_DISCOVERY_REQUEST: {
            int user_id = fcmsg->discoveryRequest.user_id;

            if (!state->client.is_active) {
                state->client.user_id = user_id;

                if (do_discovery_handshake(state, context, fcmsg) != 0) {
                    LOG_WARNING("Discovery handshake failed.");
                }

                free_packet(context, tcp_packet);
                fcmsg = NULL;
                tcp_packet = NULL;

                *new_client = true;
            }
            break;
        }
        case MESSAGE_TCP_RECOVERY: {
            // We wouldn't have called closesocket on this socket before, so we can safely call
            //     close regardless of what caused the socket failure without worrying about
            //     undefined behavior.
            SocketContextData *socket_context_data = (SocketContextData *)context->context;

            write_lock(&state->client.tcp_rwlock);
            destroy_socket_context(&state->client.tcp_context);
            if (!create_tcp_socket_context(&state->client.tcp_context, NULL, state->client.tcp_port,
                                           1, TCP_CONNECTION_WAIT, get_using_stun(),
                                           socket_context_data->binary_aes_private_key)) {
                LOG_WARNING("Failed TCP connection with client");
            }
            write_unlock(&state->client.tcp_rwlock);

            break;
        }
        default: {
            return -1;
        }
    }

    return 0;
}

int do_discovery_handshake(whist_server_state *state, SocketContext *context,
                           FractalClientMessage *fcmsg) {
    /*
        Perform a discovery handshake over the discovery port socket context

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            fcmsg (FractalClientMessage*): discovery message sent from client

        Returns:
            (int): 0 on success, -1 on failure
    */

    handle_client_message(state, fcmsg);

    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage *fsmsg = safe_malloc(fsmsg_size);
    memset(fsmsg, 0, sizeof(*fsmsg));
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage *reply_msg =
        (FractalDiscoveryReplyMessage *)fsmsg->discovery_reply;

    reply_msg->udp_port = state->client.udp_port;
    reply_msg->tcp_port = state->client.tcp_port;

    // Set connection ID in error monitor.
    error_monitor_set_connection_id(state->connection_id);

    // Send connection ID to client
    reply_msg->connection_id = state->connection_id;
    reply_msg->audio_sample_rate = state->sample_rate;

    LOG_INFO("Sending discovery packet");
    LOG_INFO("Fsmsg size is %d", (int)fsmsg_size);
    if (send_packet(context, PACKET_MESSAGE, (uint8_t *)fsmsg, (int)fsmsg_size, -1) < 0) {
        LOG_ERROR("Failed to send discovery reply message.");
        free(fsmsg);
        return -1;
    }

    free(fsmsg);

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
    udp_register_nack_buffer(&client->udp_context, PACKET_VIDEO, LARGEST_VIDEOFRAME_SIZE,
                             VIDEO_NACKBUFFER_SIZE);
    udp_register_nack_buffer(&client->udp_context, PACKET_AUDIO, LARGEST_AUDIOFRAME_SIZE,
                             AUDIO_NACKBUFFER_SIZE);

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

int broadcast_ack(Client *client) {
    int ret = 0;
    if (client->is_active) {
        read_lock(&client->tcp_rwlock);
        if (ack(&(client->tcp_context)) < 0) {
            ret = -1;
        }
        if (ack(&(client->udp_context)) < 0) {
            ret = -1;
        }
        read_unlock(&client->tcp_rwlock);
    }
    return ret;
}

int broadcast_udp_packet(Client *client, FractalPacketType type, void *data, int len,
                         int packet_id) {
    if (packet_id <= 0) {
        LOG_WARNING("Packet IDs must be positive!");
        return -1;
    }

    if (client->is_active) {
        if (send_packet(&(client->udp_context), type, data, len, packet_id) < 0) {
            LOG_WARNING("Failed to send UDP packet to client");
            return -1;
        }
    }
    return 0;
}

int broadcast_tcp_packet(Client *client, FractalPacketType type, void *data, int len) {
    if (client->is_active) {
        read_lock(&client->tcp_rwlock);
        if (send_packet(&(client->tcp_context), type, (uint8_t *)data, len, -1) < 0) {
            LOG_WARNING("Failed to send TCP packet to client");
            return -1;
        }
        read_unlock(&client->tcp_rwlock);
    }
    return 0;
}

clock last_tcp_read;
bool has_read = false;
int try_get_next_message_tcp(Client *client, FractalPacket **p_tcp_packet) {
    *p_tcp_packet = NULL;

    // Check if 20ms has passed since last TCP recvp, since each TCP recvp read takes 8ms
    bool should_recvp = false;
    if (!has_read || get_timer(last_tcp_read) * 1000.0 > 20.0) {
        should_recvp = true;
        start_timer(&last_tcp_read);
        has_read = true;
    }

    FractalPacket *tcp_packet = read_packet(&client->tcp_context, should_recvp);
    if (tcp_packet) {
        LOG_INFO("Received TCP Packet: Size %d", tcp_packet->payload_size);
        *p_tcp_packet = tcp_packet;
    }
    return 0;
}

int try_get_next_message_udp(Client *client, FractalClientMessage *fcmsg, size_t *fcmsg_size) {
    *fcmsg_size = 0;

    memset(fcmsg, 0, sizeof(*fcmsg));

    FractalPacket *packet = read_packet(&(client->udp_context), true);
    if (packet) {
        memcpy(fcmsg, packet->data, max(sizeof(*fcmsg), (size_t)packet->payload_size));
        if (packet->payload_size != get_fcmsg_size(fcmsg)) {
            LOG_WARNING("Packet is of the wrong size!: %d", packet->payload_size);
            LOG_WARNING("Type: %d", fcmsg->type);
            free_packet(&client->udp_context, packet);
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
                free_packet(&client->udp_context, packet);
                return 0;
            }
        }

        free_packet(&client->udp_context, packet);
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

int multithreaded_manage_client(void *opaque) {
    whist_server_state *state = (whist_server_state *)opaque;
    whist_server_config *config = state->config;

    SocketContext discovery_context;
    bool new_client;

    clock last_update_timer;
    start_timer(&last_update_timer);

    state->connection_id = rand();

    bool first_client_connected = false;  // set to true once the first client has connected
    bool disable_timeout = false;
    if (config->begin_time_to_exit == -1) {
        // client has `begin_time_to_exit` seconds to connect when the server first goes up.
        // If the variable is -1, disable auto-exit.
        disable_timeout = true;
    }
    clock first_client_timer;  // start this now and then discard when first client has connected
    start_timer(&first_client_timer);

    clock last_ping_check;
    start_timer(&last_ping_check);

    while (!state->exiting) {
        if (state->sample_rate == -1) {
            // If audio hasn't initialized yet, let's wait a bit.
            fractal_sleep(25);
            continue;
        }

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

        // If there is an active client that is not actively deactivating that hasn't pinged
        //     in a while, we should reap it.
        if (state->client.is_active && !state->client.is_deactivating) {
            if (reap_timed_out_client(&state->client, CLIENT_PING_TIMEOUT_SEC) != 0) {
                LOG_ERROR("Failed to reap timed out clients.");
            }
            start_timer(&last_ping_check);
        }

        if (!state->client.is_active) {
            state->connection_id = rand();

            // container exit logic -
            //  * client has connected before but now none are connected
            //  * client has not connected in `begin_time_to_exit` secs of server being up
            // We don't place this in a lock because:
            //  * if the first client connects right on the threshold of begin_time_to_exit, it
            //  doesn't matter if we disconnect
            if (!disable_timeout && (first_client_connected || (get_timer(first_client_timer) >
                                                                config->begin_time_to_exit))) {
                state->exiting = true;
            }
        }

        // Even without multiclient, we need this for TCP recovery over the discovery port
        if (!create_tcp_socket_context(&discovery_context, NULL, PORT_DISCOVERY, 1,
                                       TCP_CONNECTION_WAIT, get_using_stun(),
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

        // A client has connected and we want to start the ping timer
        start_timer(&(state->client.last_ping));

        // When a new client has been connected, we want all threads to hold client active again
        reset_threads_holding_active_count(&state->client);
        state->client.is_active = true;
    }

    return 0;
}
