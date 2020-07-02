#include "network.h"

#include "../fractal/core/fractal.h"
#include "main.h"
#include "network.h"
#include "desktop_utils.h"

// Data
extern char filename[300];
extern char username[50];
extern int UDP_port;
extern int TCP_port;
extern int client_id;
extern SocketContext PacketSendContext;
extern SocketContext PacketReceiveContext;
extern SocketContext PacketTCPContext;
extern char *server_ip;
extern bool received_server_init_message;
extern int uid;

bool using_stun;

int discoverPorts(void) {
    SocketContext context;
    using_stun = true;
    if (CreateTCPContext(&context, server_ip, PORT_DISCOVERY, 1, 500,
                         using_stun) < 0) {
        using_stun = false;
        if (CreateTCPContext(&context, server_ip, PORT_DISCOVERY, 1, 750,
                             using_stun) < 0) {
            LOG_WARNING("Failed to connect to server's discovery port.");
            return -1;
        }
    }

    FractalClientMessage fcmsg = {0};
    fcmsg.type = MESSAGE_DISCOVERY_REQUEST;
    fcmsg.discoveryRequest.username = uid;

    if (SendTCPPacket(&context, PACKET_MESSAGE, (uint8_t *)&fcmsg,
                      (int)sizeof(fcmsg)) < 0) {
        LOG_ERROR("Failed to send discovery request message.");
        closesocket(context.s);
        return -1;
    }
    LOG_INFO("Sent discovery packet");

    FractalPacket *packet;
    clock timer;
    StartTimer(&timer);
    do {
        packet = ReadTCPPacket(&context);
        SDL_Delay(5);
    } while (packet == NULL && GetTimer(timer) < 5.0);

    if (packet == NULL) {
        LOG_WARNING("Did not receive discovery packet from server.");
        closesocket(context.s);
        return -1;
    }

    FractalServerMessage *fsmsg = (FractalServerMessage *)packet->data;
    if (packet->payload_size !=
        sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage)) {
        LOG_ERROR(
            "Incorrect discovery reply message size. Expected: %d, Received: "
            "%d",
            sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage),
            packet->payload_size);
    }
    if (fsmsg->type != MESSAGE_DISCOVERY_REPLY) {
        LOG_ERROR("Message not of discovery reply type (Type: %d)",
                  fsmsg->type);
        closesocket(context.s);
        return -1;
    }

    LOG_INFO("Received discovery info packet from server!");

    FractalDiscoveryReplyMessage *reply_msg =
        (FractalDiscoveryReplyMessage *)fsmsg->discovery_reply;
    if (reply_msg->client_id == -1) {
        LOG_ERROR("Not awarded a client id from server.");
        closesocket(context.s);
        return -1;
    }

    client_id = reply_msg->client_id;
    UDP_port = reply_msg->UDP_port;
    TCP_port = reply_msg->TCP_port;
    LOG_INFO("Assigned client ID: %d. UDP Port: %d, TCP Port: %d", client_id,
             UDP_port, TCP_port);

    memcpy(filename, reply_msg->filename,
           min(sizeof(filename), sizeof(reply_msg->filename)));
    memcpy(username, reply_msg->username,
           min(sizeof(username), sizeof(reply_msg->username)));

    if (logConnectionID(reply_msg->connection_id) < 0) {
        LOG_ERROR("Failed to log connection ID.");
        closesocket(context.s);
        return -1;
    }

    received_server_init_message = true;
    closesocket(context.s);

    return 0;
}

// must be called after
int connectToServer(void) {
    if (UDP_port < 0) {
        LOG_ERROR("Trying to connect UDP but port not set.");
        return -1;
    }
    if (TCP_port < 0) {
        LOG_ERROR("Trying to connect TCP but port not set.");
        return -1;
    }

    if (CreateUDPContext(&PacketSendContext, server_ip, UDP_port, 10, 500,
                         using_stun) < 0) {
        LOG_WARNING("Failed establish UDP connection from server");
        return -1;
    }

    int a = 65535;
    if (setsockopt(PacketSendContext.s, SOL_SOCKET, SO_RCVBUF, (const char *)&a,
                   sizeof(int)) == -1) {
        LOG_ERROR("Error setting socket opts: %s\n", strerror(errno));
        return -1;
    }

    if (CreateTCPContext(&PacketTCPContext, server_ip, TCP_port, 1, 750,
                         using_stun) < 0) {
        LOG_ERROR("Failed to establish TCP connection with server.");
        closesocket(PacketSendContext.s);
        return -1;
    }

    PacketReceiveContext = PacketSendContext;

    return 0;
}

int closeConnections(void) {
    closesocket(PacketSendContext.s);
    closesocket(PacketReceiveContext.s);
    closesocket(PacketTCPContext.s);
    return 0;
}

int sendServerQuitMessages(int num_messages) {
    FractalClientMessage fmsg = {0};
    fmsg.type = CMESSAGE_QUIT;
    int retval = 0;
    for (; num_messages > 0; num_messages--) {
        SDL_Delay(50);
        if (SendFmsg(&fmsg) != 0) {
            retval = -1;
        }
    }
    return retval;
}
