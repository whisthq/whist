/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file network.c
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================
connect_to_server, close_connections, and send_server_quit_messages are used to
start and end connections to the Whist server. To connect, call connect_to_server.
To disconnect, send_server_quit_messages and then close_connections.

To communicate with the server, use send_wcmsg to send Whist messages to the server. Large wcmsg's
(e.g. clipboard messages) are sent over TCP; otherwise, messages are sent over UDP.
*/

/*
============================
Includes
============================
*/

#include "network.h"

#include <whist/core/whist.h>
#include <whist/logging/error_monitor.h>
#include "whist/core/features.h"
#include "network.h"
#include "client_utils.h"
#include "audio.h"

// Data
extern volatile char client_binary_aes_private_key[16];
SocketContext packet_udp_context = {0};
SocketContext packet_tcp_context = {0};

volatile bool connected = false;

#define UDP_CONNECTION_WAIT 500  // ms
#define TCP_CONNECTION_WAIT 500  // ms
// Controls the timeouts of read_packet on UDP
// 0ms hurts laptop batteries, but 1ms keeps update_video/update_audio live
#define UDP_CONNECTION_TIMEOUT 1  // ms
// Controls the timeout of read_packet on TCP
#define TCP_CONNECTION_TIMEOUT 1  // ms

/*
============================
Public Function Implementations
============================
*/

int connect_to_server(const char *server_ip, bool with_stun, const char *user_email) {
    LOG_INFO("using stun is %d", with_stun);

    // Connect over UDP first,
    if (!create_udp_socket_context(&packet_udp_context, (char *)server_ip, BASE_UDP_PORT,
                                   UDP_CONNECTION_TIMEOUT, UDP_CONNECTION_WAIT, with_stun,
                                   (char *)client_binary_aes_private_key)) {
        LOG_WARNING("Failed to establish UDP connection from server");
        return -1;
    }

    LOG_INFO("create_udp_socket_context() done");

    // Then connect over TCP
    if (!create_tcp_socket_context(&packet_tcp_context, (char *)server_ip, BASE_TCP_PORT,
                                   TCP_CONNECTION_TIMEOUT, TCP_CONNECTION_WAIT, with_stun,
                                   (char *)client_binary_aes_private_key)) {
        LOG_WARNING("Failed to establish TCP connection with server.");
        destroy_socket_context(&packet_udp_context);
        return -1;
    }

    LOG_INFO("create_tcp_socket_context() done");

    // Construct init packet
    WhistClientMessage wcmsg = {0};
    wcmsg.type = CMESSAGE_INIT;
    // Copy email
    if (!safe_strncpy(wcmsg.init_message.user_email, user_email,
                      sizeof(wcmsg.init_message.user_email))) {
        LOG_ERROR("User email is too long: %s.\n", user_email);
        destroy_socket_context(&packet_udp_context);
        destroy_socket_context(&packet_tcp_context);
        return -1;
    }
    // Let the server know what OS we are
#ifdef _WIN32
    wcmsg.init_message.os = WHIST_WINDOWS;
#elif defined(__APPLE__)
    wcmsg.init_message.os = WHIST_APPLE;
#else
    wcmsg.init_message.os = WHIST_LINUX;
#endif

    // Send the init packet
    if (send_packet(&packet_tcp_context, PACKET_MESSAGE, (uint8_t *)&wcmsg, (int)sizeof(wcmsg), -1,
                    false) < 0) {
        LOG_ERROR("Failed to send init message.");
        destroy_socket_context(&packet_udp_context);
        destroy_socket_context(&packet_tcp_context);
        return -1;
    }
    LOG_INFO("Sent init packet");

    // Receive init reply packet from server
    WhistPacket *tcp_packet = NULL;
    WhistTimer timer;
    start_timer(&timer);
    while (tcp_packet == NULL && get_timer(&timer) < TCP_CONNECTION_WAIT) {
        socket_update(&packet_tcp_context);
        tcp_packet = get_packet(&packet_tcp_context, PACKET_MESSAGE);
        whist_sleep(5);
    }

    // If no tcp packet was found, just return -1
    // Otherwise, parse the tcp packet's ServerInitReplyMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive init reply message from server.");
        destroy_socket_context(&packet_udp_context);
        destroy_socket_context(&packet_tcp_context);
        return -1;
    }

    // The first message received over TCP, is be guaranteed to be an init message
    FATAL_ASSERT(tcp_packet->payload_size == sizeof(WhistServerMessage));
    WhistServerMessage *wsmsg = (WhistServerMessage *)tcp_packet->data;
    FATAL_ASSERT(wsmsg->type == SMESSAGE_INIT_REPLY);

    // Parse and process the ServerInitReplyMessage
    ServerInitReplyMessage *reply_msg = (ServerInitReplyMessage *)&wsmsg->init_reply;

    // Set the connection ID
    LOG_INFO("Connection ID: %d", reply_msg->connection_id);
    whist_error_monitor_set_connection_id(reply_msg->connection_id);

    // Compare the git revision between client and server
    const char *client_revision = whist_git_revision();
    if (strncmp(client_revision, reply_msg->git_revision, sizeof(reply_msg->git_revision) - 1)) {
        LOG_WARNING("Server git revision %s does not match client revision %s.",
                    reply_msg->git_revision, client_revision);
    }

    // Match our feature mask to the server's feature mask
    whist_apply_feature_mask(reply_msg->feature_mask);

    // Free the init reply packet
    free_packet(&packet_tcp_context, tcp_packet);

    LOG_INFO("Received init reply packet from server!");

    return 0;
}

int close_connections(void) {
    /*
        Close all connections between client and server

        Return:
            (int): 0 on success
    */

    destroy_socket_context(&packet_udp_context);
    destroy_socket_context(&packet_tcp_context);
    return 0;
}

int send_server_quit_messages(int num_messages) {
    /*
        Send quit message to the server

        Arguments:
            num_messages (int): the number of quit packets
                to send to the server

        Return:
            (int): 0 on success, -1 on failure
    */

    WhistClientMessage wcmsg = {0};
    wcmsg.type = CMESSAGE_QUIT;
    int retval = 0;
    for (; num_messages > 0; num_messages--) {
        if (send_wcmsg(&wcmsg) != 0) {
            retval = -1;
        }
        whist_sleep(50);
    }
    return retval;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int send_wcmsg(WhistClientMessage *wcmsg) {
    /*
        We send large and order-sensitive wcmsg's over TCP.
        Currently, sending WhistClientMessage packets over UDP that require multiple
        sub-packets to send is not supported (if low latency large
        WhistClientMessage packets are needed, then this will have to be
        implemented)
        Also any message larger than sizeof(WhistClientMessage) should mandatorily use TCP to avoid
        server-side stack overflow.

        Arguments:
            wcmsg (WhistClientMessage*): pointer to WhistClientMessage to be send

        Return:
            (int): 0 on success, -1 on failure
    */

    // Shouldn't overflow, will take 50 days at 1000 wcmsg/second to overflow
    static unsigned int wcmsg_id = 0;
    int wcmsg_size = get_wcmsg_size(wcmsg);
    wcmsg->id = wcmsg_id;
    wcmsg_id++;

    if (!connected) {
        return -1;
    }

    // Please be careful when editing this list!
    // Please ask the maintainers of each CMESSAGE_ type
    // before adding/removing from this list
    if (wcmsg->type == CMESSAGE_INIT || wcmsg->type == CMESSAGE_FILE_DATA ||
        wcmsg->type == CMESSAGE_FILE_METADATA || wcmsg->type == CMESSAGE_CLIPBOARD ||
        wcmsg->type == MESSAGE_DIMENSIONS || wcmsg->type == MESSAGE_CONTENT_DRAG_UPDATE ||
        (size_t)wcmsg_size > sizeof(*wcmsg)) {
        return send_packet(&packet_tcp_context, PACKET_MESSAGE, wcmsg, wcmsg_size, -1, false);
    } else {
        if ((size_t)wcmsg_size > MAX_PACKET_SIZE) {
            LOG_ERROR("Attempting to send WMSG of type %d over UDP, but message is too large.",
                      wcmsg->type);
            return -1;
        }
        static int sent_packet_id = 0;
        sent_packet_id++;

        return send_packet(&packet_udp_context, PACKET_MESSAGE, wcmsg, wcmsg_size, sent_packet_id,
                           false);
    }
}
