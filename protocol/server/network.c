/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
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
#include "handle_client_message.h"

#define UDP_CONNECTION_WAIT 1000
#define TCP_CONNECTION_WAIT 1000
#define BITS_IN_BYTE 8.0

extern Client client;
extern char binary_aes_private_key[16];
extern volatile int connection_id;
extern volatile bool exiting;
extern int sample_rate;
extern bool client_joined_after_window_name_broadcast;
extern volatile FractalOSType client_os;
int begin_time_to_exit = 60;

int last_input_id = -1;

/*
============================
Private Functions
============================
*/

int handle_discovery_port_message(SocketContext *context, bool *new_client);
int do_discovery_handshake(SocketContext *context, FractalClientMessage *fcmsg);

/*
============================
Private Function Implementations
============================
*/

int handle_discovery_port_message(SocketContext *context, bool *new_client) {
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
        tcp_packet = read_tcp_packet(context, true);
        fractal_sleep(5);
    } while (tcp_packet == NULL && get_timer(timer) < CLIENT_PING_TIMEOUT_SEC);
    // Exit on null tcp packet, otherwise analyze the resulting FractalClientMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive request over discovery port from client.");
        closesocket(context->socket);
        return -1;
    }

    FractalClientMessage *fcmsg = (FractalClientMessage *)tcp_packet->data;
    *new_client = false;

    switch (fcmsg->type) {
        case MESSAGE_DISCOVERY_REQUEST: {
            int user_id = fcmsg->discoveryRequest.user_id;

            if (!client.is_active) {
                client.user_id = user_id;

                if (do_discovery_handshake(context, fcmsg) != 0) {
                    LOG_WARNING("Discovery handshake failed.");
                }

                free_tcp_packet(tcp_packet);
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
            write_lock(&client.tcp_rwlock);
            closesocket(client.tcp_context.socket);
            if (create_tcp_context(&(client.tcp_context), NULL, client.tcp_port, 1,
                                   TCP_CONNECTION_WAIT, get_using_stun(),
                                   binary_aes_private_key) < 0) {
                LOG_WARNING("Failed TCP connection with client");
            }
            write_unlock(&client.tcp_rwlock);

            break;
        }
        default: {
            return -1;
        }
    }

    return 0;
}

int do_discovery_handshake(SocketContext *context, FractalClientMessage *fcmsg) {
    /*
        Perform a discovery handshake over the discovery port socket context

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            fcmsg (FractalClientMessage*): discovery message sent from client

        Returns:
            (int): 0 on success, -1 on failure
    */

    handle_client_message(fcmsg);

    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage *fsmsg = safe_malloc(fsmsg_size);
    memset(fsmsg, 0, sizeof(*fsmsg));
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage *reply_msg =
        (FractalDiscoveryReplyMessage *)fsmsg->discovery_reply;

    reply_msg->udp_port = client.udp_port;
    reply_msg->tcp_port = client.tcp_port;

    // Set connection ID in error monitor.
    error_monitor_set_connection_id(connection_id);

    // Send connection ID to client
    reply_msg->connection_id = connection_id;
    reply_msg->audio_sample_rate = sample_rate;

    LOG_INFO("Sending discovery packet");
    LOG_INFO("Fsmsg size is %d", (int)fsmsg_size);
    if (send_tcp_packet_from_payload(context, PACKET_MESSAGE, (uint8_t *)fsmsg, (int)fsmsg_size,
                                     -1) < 0) {
        LOG_ERROR("Failed to send discovery reply message.");
        closesocket(context->socket);
        free(fsmsg);
        return -1;
    }

    closesocket(context->socket);
    free(fsmsg);

    LOG_INFO("Discovery handshake succeeded.");
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

int connect_client(bool using_stun, char *binary_aes_private_key_input) {
    if (create_udp_context(&(client.udp_context), NULL, client.udp_port, 1, UDP_CONNECTION_WAIT,
                           using_stun, binary_aes_private_key_input) < 0) {
        LOG_ERROR("Failed UDP connection with client");
        return -1;
    }

    if (create_tcp_context(&(client.tcp_context), NULL, client.tcp_port, 1, TCP_CONNECTION_WAIT,
                           using_stun, binary_aes_private_key_input) < 0) {
        LOG_WARNING("Failed TCP connection with client");
        closesocket(client.udp_context.socket);
        return -1;
    }
    return 0;
}

int disconnect_client() {
    closesocket(client.udp_context.socket);
    network_throttler_destroy(client.udp_context.network_throttler);
    closesocket(client.tcp_context.socket);
    return 0;
}

int broadcast_ack(void) {
    int ret = 0;
    if (client.is_active) {
        read_lock(&client.tcp_rwlock);
        if (ack(&(client.tcp_context)) < 0) {
            ret = -1;
        }
        if (ack(&(client.udp_context)) < 0) {
            ret = -1;
        }
        read_unlock(&client.tcp_rwlock);
    }
    return ret;
}

int broadcast_udp_packet(FractalPacket *packet, size_t packet_size) {
    /*
        Broadcasts UDP packet `packet` of size `packet_size` to client

        Arguments:
            packet (FractalPacket*): packet to be broadcast
            packet_size (size_t): size of the packet to be broadcast

        Returns:
            (int): 0 on success, -1 on failure
    */

    if (client.is_active) {
        if (send_udp_packet(&(client.udp_context), packet, packet_size) < 0) {
            LOG_ERROR("Failed to send UDP packet to client");
            return -1;
        }
    }
    return 0;
}

int broadcast_udp_packet_from_payload(FractalPacketType type, void *data, int len, int packet_id) {
    if (packet_id <= 0) {
        LOG_WARNING("Packet IDs must be positive!");
        return -1;
    }

    if (client.is_active) {
        if (send_udp_packet_from_payload(&(client.udp_context), type, data, len, packet_id) < 0) {
            LOG_WARNING("Failed to send UDP packet to client");
            return -1;
        }
    }
    return 0;
}

int broadcast_tcp_packet_from_payload(FractalPacketType type, void *data, int len) {
    if (client.is_active) {
        read_lock(&client.tcp_rwlock);
        if (send_tcp_packet_from_payload(&(client.tcp_context), type, (uint8_t *)data, len, -1) <
            0) {
            LOG_WARNING("Failed to send TCP packet to client");
            return -1;
        }
        read_unlock(&client.tcp_rwlock);
    }
    return 0;
}

clock last_tcp_read;
bool has_read = false;
int try_get_next_message_tcp(FractalPacket **p_tcp_packet) {
    *p_tcp_packet = NULL;

    // Check if 20ms has passed since last TCP recvp, since each TCP recvp read takes 8ms
    bool should_recvp = false;
    if (!has_read || get_timer(last_tcp_read) * 1000.0 > 20.0) {
        should_recvp = true;
        start_timer(&last_tcp_read);
        has_read = true;
    }

    read_lock(&client.tcp_rwlock);
    FractalPacket *tcp_packet = read_tcp_packet(&(client.tcp_context), should_recvp);
    if (tcp_packet) {
        LOG_INFO("Received TCP Packet: Size %d", tcp_packet->payload_size);
        *p_tcp_packet = tcp_packet;
    }
    read_unlock(&client.tcp_rwlock);
    return 0;
}

int try_get_next_message_udp(FractalClientMessage *fcmsg, size_t *fcmsg_size) {
    *fcmsg_size = 0;

    memset(fcmsg, 0, sizeof(*fcmsg));

    FractalPacket *packet = read_udp_packet(&(client.udp_context), false);
    if (packet) {
        memcpy(fcmsg, packet->data, max(sizeof(*fcmsg), (size_t)packet->payload_size));
        if (packet->payload_size != get_fcmsg_size(fcmsg)) {
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

int multithreaded_manage_client(void *opaque) {
    UNUSED(opaque);

    SocketContext discovery_context;
    bool new_client;

    clock last_update_timer;
    start_timer(&last_update_timer);

    connection_id = rand();

    bool first_client_connected = false;  // set to true once the first client has connected
    bool disable_timeout = false;
    if (begin_time_to_exit ==
        -1) {  // client has `begin_time_to_exit` seconds to connect when the server first goes up.
               // If the variable is -1, disable auto-exit.
        disable_timeout = true;
    }
    clock first_client_timer;  // start this now and then discard when first client has connected
    start_timer(&first_client_timer);

    clock last_ping_check;
    start_timer(&last_ping_check);

    while (!exiting) {
        if (sample_rate == -1) {
            // If audio hasn't initialized yet, let's wait a bit.
            fractal_sleep(25);
            continue;
        }

        LOG_INFO("Is a client connected? %s", client.is_active ? "yes" : "no");

        // If all threads have stopped using the active client, we can finally quit it
        if (client.is_deactivating && !threads_still_holding_active()) {
            if (quit_client() != 0) {
                LOG_ERROR("Failed to quit client.");
            } else {
                client.is_deactivating = false;
                LOG_INFO("Successfully quit client.");
            }
        }

        // If there is an active client that is not actively deactivating that hasn't pinged
        //     in a while, we should reap it.
        if (client.is_active && !client.is_deactivating && get_timer(last_ping_check) > 20.0) {
            if (reap_timed_out_client(CLIENT_PING_TIMEOUT_SEC) != 0) {
                LOG_ERROR("Failed to reap timed out clients.");
            }
            start_timer(&last_ping_check);
        }

        if (!client.is_active) {
            connection_id = rand();

            // container exit logic -
            //  * client has connected before but now none are connected
            //  * client has not connected in `begin_time_to_exit` secs of server being up
            // We don't place this in a lock because:
            //  * if the first client connects right on the threshold of begin_time_to_exit, it
            //  doesn't matter if we disconnect
            if (!disable_timeout &&
                (first_client_connected || (get_timer(first_client_timer) > begin_time_to_exit))) {
                exiting = true;
            }
        }

        // Even without multiclient, we need this for TCP recovery over the discovery port
        if (create_tcp_context(&discovery_context, NULL, PORT_DISCOVERY, 1, TCP_CONNECTION_WAIT,
                               get_using_stun(), binary_aes_private_key) < 0) {
            continue;
        }

        // This can either be a new client connecting, or an existing client asking for a TCP
        //     connection to be recovered. We use the discovery port because it is always
        //     accepting connections and is reliable for both discovery and recovery messages.
        if (handle_discovery_port_message(&discovery_context, &new_client) != 0) {
            LOG_WARNING("Discovery port message could not be handled.");
            continue;
        }

        // If the handled message was not for a discovery handshake, then skip
        //     over everything that is necessary for setting up a new client
        if (!new_client) {
            continue;
        }

        // Client is not in use so we don't need to worry about anyone else
        // touching it
        if (connect_client(get_using_stun(), binary_aes_private_key) != 0) {
            LOG_WARNING("Failed to establish connection with client.");
            continue;
        }

        LOG_INFO("Client connected.");

        if (!client.is_active) {
            // we have went from 0 clients to 1 client, so we have got our first client
            // this variable should never be set back to false after this
            first_client_connected = true;
        }
        client_joined_after_window_name_broadcast = true;

        // We reset the input tracker when a new client connects
        reset_input(client_os);

        // A client has connected and we want to start the ping timer
        start_timer(&(client.last_ping));

        // When a new client has been connected, we want all threads to hold client active again
        reset_threads_holding_active_count();
        client.is_active = true;
    }

    return 0;
}
