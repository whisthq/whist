/**
 * Copyright Fractal Computers, Inc. 2020
 * @file network.c
 * @brief This file contains client-specific wrappers to low-level network
 *        functions.
============================
Usage
============================

Use these functions for any client-specific networking needs.
*/

/*
============================
Includes
============================
*/

#include "network.h"

#include <fractal/core/fractal.h>
#include "network.h"
#include "client_utils.h"
#include "audio.h"

// Init information
extern char user_email[FRACTAL_ARGS_MAXLEN + 1];

// Data
double latency;
extern volatile char binary_aes_private_key[16];
extern char filename[300];
extern char username[50];
extern int udp_port;
extern int tcp_port;
extern int client_id;
SocketContext packet_udp_context = {0};
SocketContext packet_tcp_context = {0};
extern char *server_ip;
extern int uid;


#define TCP_CONNECTION_WAIT 300  // ms
#define UDP_CONNECTION_WAIT 300  // ms

/*
============================
Public Function Implementations
============================
*/

int discover_ports(bool *using_stun) {
    /*
        Send a discovery packet to the server to determine which TCP
        and UDP packets are assigned to the client. Must be called
        before `connect_to_server()`.

        Arguments:
            using_stun (bool*): whether we are using the STUN server

        Return:
            (int): 0 on success, -1 on failure
    */

    // Create TCP context
    SocketContext context;
    LOG_INFO("Trying to connect (Using STUN: %s)", *using_stun ? "true" : "false");
    if (create_tcp_context(&context, server_ip, PORT_DISCOVERY, 1, TCP_CONNECTION_WAIT, *using_stun,
                           (char *)binary_aes_private_key) < 0) {
        *using_stun = !*using_stun;
        LOG_INFO("Trying to connect (Using STUN: %s)", *using_stun ? "true" : "false");
        if (create_tcp_context(&context, server_ip, PORT_DISCOVERY, 1, TCP_CONNECTION_WAIT,
                               *using_stun, (char *)binary_aes_private_key) < 0) {
            LOG_WARNING("Failed to connect to server's discovery port.");
            return -1;
        }
    }

    // Create and send discovery request packet
    FractalClientMessage fcmsg = {0};
    fcmsg.type = MESSAGE_DISCOVERY_REQUEST;
    fcmsg.discoveryRequest.user_id = uid;

    prepare_init_to_server(&fcmsg.discoveryRequest, user_email);

    if (send_tcp_packet(&context, PACKET_MESSAGE, (uint8_t *)&fcmsg, (int)sizeof(fcmsg)) < 0) {
        LOG_ERROR("Failed to send discovery request message.");
        closesocket(context.socket);
        return -1;
    }
    LOG_INFO("Sent discovery packet");

    // Receive discovery packets from server
    FractalPacket *tcp_packet = NULL;
    clock timer;
    start_timer(&timer);
    do {
        tcp_packet = read_tcp_packet(&context, true);
        SDL_Delay(5);
    } while (tcp_packet == NULL && get_timer(timer) < 5.0);
    closesocket(context.socket);

    // If no tcp packet was found, just return -1
    // Otherwise, parse the tcp packet's FractalDiscoveryReplyMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive discovery packet from server.");
        return -1;
    }

    FractalServerMessage *fsmsg = (FractalServerMessage *)tcp_packet->data;
    if (tcp_packet->payload_size !=
        sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage)) {
        LOG_ERROR(
            "Incorrect discovery reply message size. Expected: %d, Received: "
            "%d",
            sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage),
            tcp_packet->payload_size);
        free_tcp_packet(tcp_packet);
        return -1;
    }
    if (fsmsg->type != MESSAGE_DISCOVERY_REPLY) {
        LOG_ERROR("Message not of discovery reply type (Type: %d)", fsmsg->type);
        free_tcp_packet(tcp_packet);
        return -1;
    }

    LOG_INFO("Received discovery info packet from server!");

    // Create and send discovery reply message
    FractalDiscoveryReplyMessage *reply_msg =
        (FractalDiscoveryReplyMessage *)fsmsg->discovery_reply;
    if (reply_msg->client_id == -1) {
        LOG_ERROR("Not awarded a client id from server.");
        free_tcp_packet(tcp_packet);
        return -1;
    }

    client_id = reply_msg->client_id;
    set_audio_frequency(reply_msg->audio_sample_rate);
    udp_port = reply_msg->UDP_port;
    tcp_port = reply_msg->TCP_port;
    LOG_INFO("Assigned client ID: %d. UDP Port: %d, TCP Port: %d", client_id, udp_port, tcp_port);

    memcpy(filename, reply_msg->filename, min(sizeof(filename), sizeof(reply_msg->filename)));
    memcpy(username, reply_msg->username, min(sizeof(username), sizeof(reply_msg->username)));

    if (log_connection_id(reply_msg->connection_id) < 0) {
        LOG_ERROR("Failed to log connection ID.");
        free_tcp_packet(tcp_packet);
        return -1;
    }

    // fsmsg and reply_msg are pointers into tcp_packet,
    // but at this point, we're done.
    free_tcp_packet(tcp_packet);

    return 0;
}

int connect_to_server(bool using_stun) {
    /*
        Connect to the server. Must be called after `discover_ports()`.

        Arguments:
            using_stun (bool): whether we are using the STUN server

        Return:
            (int): 0 on success, -1 on failure
    */

    LOG_INFO("using stun is %d", using_stun);
    if (udp_port < 0) {
        LOG_ERROR("Trying to connect UDP but port not set.");
        return -1;
    }
    if (tcp_port < 0) {
        LOG_ERROR("Trying to connect TCP but port not set.");
        return -1;
    }

    if (create_udp_context(&packet_udp_context, server_ip, udp_port, 10, UDP_CONNECTION_WAIT,
                           using_stun, (char *)binary_aes_private_key) < 0) {
        LOG_WARNING("Failed establish UDP connection from server");
        return -1;
    }

    // socket options = TCP_NODELAY IPTOS_LOWDELAY SO_RCVBUF=65536
    // Windows Socket 65535 Socket options apply to all sockets.
    // this is set to stop the kernel from buffering too much, thereby
    // getting the data to us faster for lower latency
    int a = 65535;
    if (setsockopt(packet_udp_context.socket, SOL_SOCKET, SO_RCVBUF, (const char *)&a,
                   sizeof(int)) == -1) {
        LOG_ERROR("Error setting socket opts: %d", get_last_network_error());
        return -1;
    }

    if (create_tcp_context(&packet_tcp_context, server_ip, tcp_port, 1, TCP_CONNECTION_WAIT,
                           using_stun, (char *)binary_aes_private_key) < 0) {
        LOG_ERROR("Failed to establish TCP connection with server.");
        closesocket(packet_udp_context.socket);
        return -1;
    }

    return 0;
}

int close_connections(void) {
    /*
        Close all connections between client and server

        Return:
            (int): 0 on success
    */

    closesocket(packet_udp_context.socket);
    closesocket(packet_tcp_context.socket);
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

    FractalClientMessage fmsg = {0};
    fmsg.type = CMESSAGE_QUIT;
    int retval = 0;
    for (; num_messages > 0; num_messages--) {
        SDL_Delay(50);
        if (send_fmsg(&fmsg) != 0) {
            retval = -1;
        }
    }
    return retval;
}

int send_fmsg(FractalClientMessage *fmsg) {
    /*
        We send large fmsg's over TCP. At the moment, this is only CLIPBOARD;
        Currently, sending FractalClientMessage packets over UDP that require multiple
        sub-packets to send is not supported (if low latency large
        FractalClientMessage packets are needed, then this will have to be
        implemented)

        Arguments:
            fmsg (FractalClientMessage*): pointer to FractalClientMessage to be send

        Return:
            (int): 0 on success, -1 on failure
    */

    // Shouldn't overflow, will take 50 days at 1000 fmsg/second to overflow
    static unsigned int fmsg_id = 0;
    fmsg->id = fmsg_id;
    fmsg_id++;

    if (fmsg->type == CMESSAGE_CLIPBOARD || fmsg->type == MESSAGE_DISCOVERY_REQUEST) {
        return send_tcp_packet(&packet_tcp_context, PACKET_MESSAGE, fmsg, get_fmsg_size(fmsg));
    } else {
        if ((size_t)get_fmsg_size(fmsg) > MAX_PACKET_SIZE) {
            LOG_ERROR(
                "Attempting to send FMSG that is too large for UDP, and only CLIPBOARD and TIME is "
                "presumed to be over TCP");
            return -1;
        }
        static int sent_packet_id = 0;
        sent_packet_id++;
        return send_udp_packet(&packet_udp_context, PACKET_MESSAGE, fmsg, get_fmsg_size(fmsg),
                               sent_packet_id, -1, NULL, NULL);
    }
}

void send_message_ping() {
    fmsg.type = MESSAGE_PING;
    fmsg.ping_id = ping_id;

    LOG_INFO("Ping %d sent", ping_id);
    send_fmsg(&fmsg);
    start_timer(&last_ping_timer);
}

void handle_pong(int pong_id) {
    if (last_ping_id == pong_id) {
        double ping_time = get_timer(last_ping_timer);
        LOG_INFO("Pong %d received: round-trip time %f", pong_id, ping_time);

        // We define latency according to an exponential moving average,
        // since this is really easy to compute. The lambda=0.8 is to
        // make sure we don't over-weight the most recent ping_time.
        const double lambda = 0.8;
        latency = lambda * latency + (1.0 - lambda) * ping_time;

        ping_failures = 0;
    } else {
        LOG_INFO("Old pong %d received; expected pong %d", pong_id, last_ping_id);
    }
}

void handle_ping() {
    if (get_timer(last_ping_timer) > 1.0) {
        LOG_WARNING("No server pong received in over a second");
    }

    if (last_ping_id > last_pong_id && get_timer(last_ping_timer) > 0.6) {
        LOG_WARNING("Ping %d received no response", last_ping_id);
        last_pong_id = last_ping_id;

        ++ping_failures;
        if (ping_failures == 3) {
            LOG_ERROR("3 consecutive ping failures: disconnecting");
            connected = false;
        }
    }

    if (last_ping_id == last_pong_id && get_timer(last_ping_timer) > 0.5) {
        send_message_ping(++last_ping_id);
    }

    static last_ping_attempt_number = 0;

    if (last_ping_id != last_pong_id && get_timer(last_ping_timer) > 0.21) {
        send_message_ping(last_ping_id);
    }
}
