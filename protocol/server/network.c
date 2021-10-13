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
#include <fractal/logging/error_monitor.h>
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
extern volatile FractalOSType client_os;
int begin_time_to_exit = 60;

int last_input_id = -1;

/*
============================
Private Functions
============================
*/

int handle_discovery_port_message(SocketContext *context, int *client_id, bool *new_client);
int do_discovery_handshake(SocketContext *context, int client_id, FractalClientMessage *fcmsg);

/*
============================
Private Function Implementations
============================
*/

int handle_discovery_port_message(SocketContext *context, int *client_id, bool *new_client) {
    /*
        Handle a message from the client over received over the discovery port.

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            client_id (int*): pointer to the client ID to be populated
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

            if (do_discovery_handshake(context, *client_id, fcmsg) != 0) {
                LOG_WARNING("Discovery handshake failed.");
            }

            free_tcp_packet(tcp_packet);
            fcmsg = NULL;
            tcp_packet = NULL;

            *new_client = true;
            break;
        }
        case MESSAGE_TCP_RECOVERY: {
            *client_id = fcmsg->tcpRecovery.client_id;

            // We wouldn't have called closesocket on this socket before, so we can safely call
            //     close regardless of what caused the socket failure without worrying about
            //     undefined behavior.
            write_lock(&clients[*client_id].tcp_rwlock);
            closesocket(clients[*client_id].tcp_context.socket);
            if (create_tcp_context(&(clients[*client_id].tcp_context), NULL,
                                   clients[*client_id].tcp_port, 1, TCP_CONNECTION_WAIT,
                                   get_using_stun(), binary_aes_private_key) < 0) {
                LOG_WARNING("Failed TCP connection with client (ID: %d)", *client_id);
            }
            write_unlock(&clients[*client_id].tcp_rwlock);

            break;
        }
        default: {
            return -1;
        }
    }

    return 0;
}

int do_discovery_handshake(SocketContext *context, int client_id, FractalClientMessage *fcmsg) {
    /*
        Perform a discovery handshake over the discovery port socket context

        Arguments:
            context (SocketContext*): the socket context for the discovery port
            client_id (int): the ID of the client to perform a handshake with
            fcmsg (FractalClientMessage*): discovery message sent from client

        Returns:
            (int): 0 on success, -1 on failure
    */

    handle_client_message(fcmsg, client_id, true);

    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage *fsmsg = safe_malloc(fsmsg_size);
    memset(fsmsg, 0, sizeof(*fsmsg));
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage *reply_msg =
        (FractalDiscoveryReplyMessage *)fsmsg->discovery_reply;

    reply_msg->client_id = client_id;
    reply_msg->udp_port = clients[client_id].udp_port;
    reply_msg->tcp_port = clients[client_id].tcp_port;

    // Set connection ID in error monitor.
    error_monitor_set_connection_id(connection_id);

    // Send connection ID to client
    reply_msg->connection_id = connection_id;
    reply_msg->audio_sample_rate = sample_rate;

    LOG_INFO("Sending discovery packet");
    LOG_INFO("Fsmsg size is %d", (int)fsmsg_size);
    if (send_tcp_packet_from_payload(context, PACKET_MESSAGE, (uint8_t *)fsmsg, (int)fsmsg_size) <
        0) {
        LOG_ERROR("Failed to send send discovery reply message.");
        closesocket(context->socket);
        free(fsmsg);
        return -1;
    }

    closesocket(context->socket);
    free(fsmsg);

    LOG_INFO("Discovery handshake succeeded. (ID: %d)", client_id);
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

int connect_client(int id, bool using_stun, char *binary_aes_private_key_input) {
    if (create_udp_context(&(clients[id].udp_context), NULL, clients[id].udp_port, 1,
                           UDP_CONNECTION_WAIT, using_stun, binary_aes_private_key_input) < 0) {
        LOG_ERROR("Failed UDP connection with client (ID: %d)", id);
        return -1;
    }

    if (create_tcp_context(&(clients[id].tcp_context), NULL, clients[id].tcp_port, 1,
                           TCP_CONNECTION_WAIT, using_stun, binary_aes_private_key_input) < 0) {
        LOG_WARNING("Failed TCP connection with client (ID: %d)", id);
        closesocket(clients[id].udp_context.socket);
        return -1;
    }
    return 0;
}

int disconnect_client(int id) {
    closesocket(clients[id].udp_context.socket);
    network_throttler_destroy(clients[id].udp_context.network_throttler);
    closesocket(clients[id].tcp_context.socket);
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
            read_lock(&clients[id].tcp_rwlock);
            ack(&(clients[id].tcp_context));
            ack(&(clients[id].udp_context));
            read_unlock(&clients[id].tcp_rwlock);
        }
    }
    return ret;
}

int broadcast_udp_packet(FractalPacket *packet, size_t packet_size) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            if (send_udp_packet(&(clients[id].udp_context), packet, packet_size) < 0) {
                LOG_ERROR("Failed to send UDP packet to client (ID: %d)", id);
                ret = -1;
            }
        }
    }
    return ret;
}

int broadcast_udp_packet_from_payload(FractalPacketType type, void *data, int len, int packet_id) {
    if (packet_id <= 0) {
        LOG_WARNING("Packet IDs must be positive!");
        return -1;
    }

    int ret = 0;
    for (int j = 0; j < MAX_NUM_CLIENTS; ++j) {
        if (clients[j].is_active) {
            if (send_udp_packet_from_payload(&(clients[j].udp_context), type, data, len,
                                             packet_id) < 0) {
                LOG_WARNING("Failed to send UDP packet to client id: %d", j);
                ret = -1;
            }
        }
    }
    return ret;
}

int broadcast_tcp_packet_from_payload(FractalPacketType type, void *data, int len) {
    int ret = 0;
    for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
        if (clients[id].is_active) {
            read_lock(&clients[id].tcp_rwlock);
            if (send_tcp_packet_from_payload(&(clients[id].tcp_context), type, (uint8_t *)data,
                                             len) < 0) {
                LOG_WARNING("Failed to send TCP packet to client id: %d", id);
                ret = -1;
            }
            read_unlock(&clients[id].tcp_rwlock);
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

    read_lock(&clients[client_id].tcp_rwlock);
    FractalPacket *tcp_packet = read_tcp_packet(&(clients[client_id].tcp_context), should_recvp);
    if (tcp_packet) {
        LOG_INFO("Received TCP Packet (Probably clipboard): Size %d", tcp_packet->payload_size);
        *p_tcp_packet = tcp_packet;
    }
    read_unlock(&clients[client_id].tcp_rwlock);
    return 0;
}

int try_get_next_message_udp(int client_id, FractalClientMessage *fcmsg, size_t *fcmsg_size) {
    *fcmsg_size = 0;

    memset(fcmsg, 0, sizeof(*fcmsg));

    FractalPacket *packet = read_udp_packet(&(clients[client_id].udp_context));
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

int multithreaded_manage_clients(void *opaque) {
    UNUSED(opaque);

    SocketContext discovery_context;
    int client_id;
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

    while (!exiting) {
        if (sample_rate == -1) {
            // If audio hasn't initialized yet, let's wait a bit.
            fractal_sleep(25);
            continue;
        }
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

        // This can either be a new client connecting, or an existing client asking for a TCP
        //     connection to be recovered. We use the discovery port because it is always
        //     accepting connections and is reliable for both discovery and recovery messages.
        if (handle_discovery_port_message(&discovery_context, &client_id, &new_client) != 0) {
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
            reset_input(client_os);
        }

        start_timer(&(clients[client_id].last_ping));

        clients[client_id].is_active = true;

        write_unlock(&is_active_rwlock);
    }

    return 0;
}
