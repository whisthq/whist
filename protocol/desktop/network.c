#include "../fractal/core/fractal.h"
#include "main.h"
#include "network.h"

extern SocketContext PacketSendContext;
extern SocketContext PacketReceiveContext;
extern SocketContext PacketTCPContext;
extern char *server_ip;
extern bool received_server_init_message;

int tcp_timeout = 250;

static int connectUDPOutgoing(int port, bool using_stun) {
    if (CreateUDPContext(&PacketSendContext, (char *)server_ip,
            port, 10, 500, using_stun) < 0) {
        LOG_WARNING("Failed establish outgoing UDP connection to server");
        return -1;
    }
    return 0;
}

static int connectUDPIncoming(int port, bool using_stun) {
    if (CreateUDPContext(&PacketReceiveContext, (char *)server_ip,
                        port, 1, 500, using_stun) < 0) {
        LOG_WARNING("Failed establish incoming UDP connection from server");
        return -1;
    }

    int a = 65535;
    if (setsockopt(PacketReceiveContext.s, SOL_SOCKET, SO_RCVBUF,
                   (const char*)&a, sizeof(int)) == -1) {
        LOG_ERROR("Error setting socket opts: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int connectTCP(int port, bool using_stun) {
    if (CreateTCPContext(&PacketTCPContext, (char *)server_ip, port,
                        1, tcp_timeout, using_stun) < 0) {
        return -1;
    }
    return 0;
}


static int getSpectatorServerPort(void) {
    int port;
    bool using_stun = true;

    if (connectUDPIncoming(PORT_SPECTATOR, using_stun) != 0) {
        using_stun = false;
        if (connectUDPIncoming(PORT_SPECTATOR, using_stun) != 0) {
            return -1;
        }
    }

    FractalPacket *init_spectator;
    clock init_spectator_timer;
    StartTimer( &init_spectator_timer );
    do {
        SDL_Delay(5);
        init_spectator = ReadUDPPacket(&PacketReceiveContext);
    } while (init_spectator == NULL && GetTimer(init_spectator_timer) < 1.0);

    if (init_spectator == NULL) {
        LOG_ERROR("Did not receive spectator init packet from server.");
        closesocket(PacketReceiveContext.s);
        return -1;

    }

    FractalServerMessage *fmsg = (FractalServerMessage *) init_spectator->data;
    port = fmsg->spectator_port;

    closesocket(PacketReceiveContext.s);

    return port;
}

int establishSpectatorConnections(void) {
    int server_port = getSpectatorServerPort();
    if (server_port < 0) {
        LOG_ERROR("Failed to get spectator's port on server.");
        return -1;
    }

    bool using_stun = true;

    if (connectUDPOutgoing(server_port, using_stun) != 0) {
        using_stun = false;
        if (connectUDPOutgoing(server_port, using_stun) != 0) {
            return -1;
        }
    }
    PacketReceiveContext = PacketSendContext;
    return 0;
}

int closeSpectatorConnections(void) {
    closesocket(PacketSendContext.s);
    return 0;
}

int establishConnections(void) {
    bool using_stun = true;

    if (connectUDPOutgoing(PORT_CLIENT_TO_SERVER, using_stun) != 0) {
        using_stun = false;
        if (connectUDPOutgoing(PORT_CLIENT_TO_SERVER, using_stun) != 0) {
            return -1;
        }
    }

    SDL_Delay(150);

    if (connectUDPIncoming(PORT_SERVER_TO_CLIENT, using_stun) != 0) {
        closesocket(PacketSendContext.s);
        return -1;
    }

    SDL_Delay(150);

    if (connectTCP(PORT_SHARED_TCP, using_stun) != 0) {
        tcp_timeout += 250;
        closesocket(PacketSendContext.s);
        closesocket(PacketReceiveContext.s);
        return -1;
    }
    return 0;
}

int closeConnections(void) {
    closesocket(PacketSendContext.s);
    closesocket(PacketReceiveContext.s);
    closesocket(PacketTCPContext.s);
    return 0;
}

int waitForServerInitMessage(int timeout) {
    clock waiting_for_init_timer;
    StartTimer(&waiting_for_init_timer);
    while (!received_server_init_message) {
        // If 500ms and no init timer was received, we should disconnect
        // because something failed
        if (GetTimer(waiting_for_init_timer) > timeout / 1000.0) {
            LOG_ERROR("Took too long for init timer!");
            return -1;
        }
        SDL_Delay(25);
    }
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
