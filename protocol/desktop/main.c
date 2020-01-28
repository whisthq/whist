#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <process.h>
    #include <windows.h>
    #include <synchapi.h>
    #pragma comment (lib, "ws2_32.lib")
#else
    #include <unistd.h>
#endif

#include "../include/fractal.h"
#include "../include/webserver.h" // header file for webserver query functions
#include "../include/linkedlist.h" // header file for audio decoder

#include "video.h"
#include "audio.h"

// Width and Height
volatile int server_width = -1;
volatile int server_height = -1;

// maximum mbps
volatile double max_mbps = START_MAX_MBPS;
volatile bool update_mbps = false;

// Global state variables
volatile static bool run_receive_packets = false;
volatile static bool is_timing_latency = false;
volatile static clock latency_timer;
volatile static int ping_id = 1;
volatile static int ping_failures = 0;

// Function Declarations

int SendPacket(struct SocketContext* context, void* data, int len);
static int32_t ReceivePackets(void* opaque);
static int32_t ReceiveMessage(struct RTPPacket* packet, int recv_size);

int SendPacket(struct SocketContext* context, void* data, int len) {
    if (sendto(context->s, data, len, 0, (struct sockaddr*)(&context->addr), sizeof(context->addr)) < 0) {
        mprintf("Failed to send packet!\n");
        return -1;
    }
    return 0;
}

static int32_t ReceivePackets(void* opaque) {
    mprintf("ReceivePackets running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    struct RTPPacket packet = { 0 };
    struct SocketContext socketContext = *(struct SocketContext*) opaque;
    int slen = sizeof(socketContext.addr);

    SendAck(&socketContext, 1);

    clock ack_timer;
    StartTimer(&ack_timer);
    for (int i = 0; run_receive_packets; i++) {
        // Call as often as possible
        updateVideo();

        int recv_size = recvfrom(socketContext.s, &packet, sizeof(packet), 0, (struct sockaddr*)(&socketContext.addr), &slen);
        int packet_size = sizeof(packet) - sizeof(packet.data) + packet.payload_size;

        if (recv_size < 0) {
            int error = WSAGetLastError();
            switch (error) {
            case WSAETIMEDOUT:
            case WSAEWOULDBLOCK:
                break;
            default:
                mprintf("Unexpected Packet Error: %d\n", error);
                break;
            }
        }
        else if (packet.payload_size < 0 || packet_size != recv_size) {
            mprintf("Invalid packet size\nPayload Size: %d\nPacket Size: %d\nRecv_size: %d\n", packet_size, recv_size, packet.payload_size);
        }
        else {
            uint32_t hash = Hash((char*)&packet + sizeof(packet.hash), packet_size - sizeof(packet.hash));
            if (hash != packet.hash) {
                mprintf("Incorrect Hash\n");
            }
            else {
                switch (packet.type) {
                case PACKET_AUDIO:
                    ReceiveAudio(&packet, recv_size);
                    break;
                case PACKET_VIDEO:
                    ReceiveVideo(&packet, recv_size);
                    break;
                case PACKET_MESSAGE:
                    ReceiveMessage(&packet, recv_size);
                    break;
                }
            }
        }

        if (GetTimer(ack_timer) * 1000.0 > ACK_REFRESH_MS) {
            SendAck(&socketContext, 1);
            StartTimer(&ack_timer);
        }
    }

    SDL_Delay(5);
}

static int32_t ReceiveMessage(struct RTPPacket* packet, int recv_size) {
    FractalServerMessage fmsg = *(FractalServerMessage*)packet->data;
    switch (fmsg.type) {
    case MESSAGE_PONG:
        if (ping_id == fmsg.ping_id) {
            mprintf("Latency: %f\n", GetTimer(latency_timer));
            is_timing_latency = false;
            ping_failures = 0;
        }
        else {
            mprintf("Old Ping ID found.\n");
        }
        break;
    default:
        mprintf("Unknown Server Message Received\n");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    initMultiThreadedPrintf();

    // initialize the windows socket library if this is a windows client
#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }
#endif

    SDL_Event msg;
    FractalClientMessage fmsg = { 0 };

    struct SocketContext PacketSendContext = { 0 };
    if (CreateUDPContext(&PacketSendContext, "C", SERVER_IP, 10, 250) < 0) {
        exit(1);
    }

    struct SocketContext PacketReceiveContext = { 0 };
    if (CreateUDPContext(&PacketReceiveContext, "C", SERVER_IP, 0, 250) < 0) {
        exit(1);
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    initVideo();
    initAudio();

    run_receive_packets = true;
    SDL_Thread* receive_packets_thread;
    receive_packets_thread = SDL_CreateThread(ReceivePackets, "ReceivePackets", &PacketReceiveContext);

    StartTimer(&latency_timer);

    bool needs_dimension_update = true;
    bool tried_to_update_dimension = false;
    clock last_dimension_update;
    StartTimer(&last_dimension_update);

    bool shutting_down = false;
    while (!shutting_down)
    {
        if (needs_dimension_update && !tried_to_update_dimension && (server_width != OUTPUT_WIDTH || server_height != OUTPUT_HEIGHT)) {
            mprintf("Sending dim message!\n");
            memset(&fmsg, 0, sizeof(fmsg));
            fmsg.type = MESSAGE_DIMENSIONS;
            fmsg.width = OUTPUT_WIDTH;
            fmsg.height = OUTPUT_HEIGHT;
            SendPacket(&PacketSendContext, &fmsg, sizeof(fmsg));
            tried_to_update_dimension = true;
        }

        if (update_mbps) {
            mprintf("Updating MBPS: %f\n", max_mbps);
            update_mbps = false;
            memset(&fmsg, 0, sizeof(fmsg));
            fmsg.type = MESSAGE_MBPS;
            fmsg.mbps = max_mbps;
            SendPacket(&PacketSendContext, &fmsg, sizeof(fmsg));
        }

        if (is_timing_latency && GetTimer(latency_timer) > 0.5) {
            mprintf("Ping received no response.\n");
            is_timing_latency = false;
            ping_failures++;
            if (ping_failures == 3) {
                mprintf("Server disconnected: 3 consecutive ping failures.\n");
                shutting_down = true;
                break;
            }
        }

        if (!is_timing_latency && GetTimer(latency_timer) > 0.5) {
            memset(&fmsg, 0, sizeof(fmsg));
            ping_id++;
            fmsg.type = MESSAGE_PING;
            fmsg.ping_id = ping_id;
            is_timing_latency = true;

            StartTimer(&latency_timer);
            SendPacket(&PacketSendContext, &fmsg, sizeof(fmsg));
        }

        memset(&fmsg, 0, sizeof(fmsg));
        if (SDL_PollEvent(&msg)) {
            switch (msg.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                fmsg.type = MESSAGE_KEYBOARD;
                fmsg.keyboard.code = (FractalKeycode)msg.key.keysym.scancode;
                fmsg.keyboard.mod = msg.key.keysym.mod;
                fmsg.keyboard.pressed = msg.key.type == SDL_KEYDOWN;
                break;
            case SDL_MOUSEMOTION:
                fmsg.type = MESSAGE_MOUSE_MOTION;
                fmsg.mouseMotion.x = msg.motion.x * server_width / OUTPUT_WIDTH;
                fmsg.mouseMotion.y = msg.motion.y * server_height / OUTPUT_HEIGHT;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                fmsg.type = MESSAGE_MOUSE_BUTTON;
                fmsg.mouseButton.button = msg.button.button;
                fmsg.mouseButton.pressed = msg.button.type == SDL_MOUSEBUTTONDOWN;
                break;
            case SDL_MOUSEWHEEL:
                fmsg.type = MESSAGE_MOUSE_WHEEL;
                fmsg.mouseWheel.x = msg.wheel.x;
                fmsg.mouseWheel.y = msg.wheel.y;
                break;
            case SDL_QUIT:
                mprintf("Quitting...\n");
                fmsg.type = MESSAGE_QUIT;
                shutting_down = true;
                break;
            }
            if (fmsg.type != 0) {
                SendPacket(&PacketSendContext, &fmsg, sizeof(fmsg));
            }
        }
    }

    run_receive_packets = false;
    SDL_WaitThread(receive_packets_thread, NULL);

    destroyVideo();
    destroyAudio();

    SDL_Quit();
    
#if defined(_WIN32)
    closesocket(PacketSendContext.s);
    closesocket(PacketReceiveContext.s);
    WSACleanup();
#else
    close(PacketSendContext.s);
    close(PacketReceiveContext.s);
#endif

    destroyMultiThreadedPrintf();
    printf("Closing Client...\n");

    return 0;
}
