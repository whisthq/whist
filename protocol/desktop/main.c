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
    #include <errno.h>

    // adapt windows error codes
    #define WSAETIMEDOUT ETIMEDOUT
    #define WSAEWOULDBLOCK EWOULDBLOCK
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

SDL_mutex* send_packet_mutex;
int SendPacket(void* data, int len);
static int32_t ReceivePackets(void* opaque);
static int32_t ReceiveMessage(struct RTPPacket* packet, int recv_size);

struct SocketContext PacketSendContext;
int SendPacket(void* data, int len) {
    if (len > MAX_PACKET_SIZE) {
        mprintf("Packet too large!\n");
        return -1;
    }

    bool failed = false;

    SDL_LockMutex(send_packet_mutex);
    if (sendto(PacketSendContext.s, data, len, 0, (struct sockaddr*)(&PacketSendContext.addr), sizeof(PacketSendContext.addr)) < 0) {
        mprintf("Failed to send packet!\n");
        failed = true;
    }
    SDL_UnlockMutex(send_packet_mutex);

    return failed ? -1 : 0;
}

static int32_t ReceivePackets(void* opaque) {
    mprintf("ReceivePackets running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    struct RTPPacket packet = { 0 };
    struct SocketContext socketContext = *(struct SocketContext*) opaque;
    int slen = sizeof(socketContext.addr);

    int total_recvs = 0;


    /****
    Timers
    ****/
    clock world_timer;
    StartTimer(&world_timer);

    double recvfrom_time = 0;
    double update_video_time = 0;
    double update_audio_time = 0;
    double hash_time = 0;
    double video_time = 0;
    double audio_time = 0;
    double message_time = 0;

    double max_audio_time = 0;
    double max_video_time = 0;

    clock recvfrom_timer;
    clock update_video_timer;
    clock update_audio_timer;
    clock hash_timer;
    clock video_timer;
    clock audio_timer;
    clock message_timer;

    /****
    End Timers
    ****/

    for (int i = 0; run_receive_packets; i++) {
        //mprintf("Update\n");
        // Call as often as possible
        if (GetTimer(world_timer) > 5) {
            mprintf("\nworld_time: %f\n", GetTimer(world_timer));
            mprintf("recvfrom_time: %f\n", recvfrom_time);
            mprintf("update_video_time: %f\n", update_video_time);
            mprintf("update_audio_time: %f\n", update_audio_time);
            mprintf("hash_time: %f\n", hash_time);
            mprintf("video_time: %f\n", video_time);
            mprintf("max_video_time: %f\n", max_video_time);
            mprintf("audio_time: %f\n", audio_time);
            mprintf("max_audio_time: %f\n", max_audio_time);
            mprintf("message_time: %f\n", message_time);
            StartTimer(&world_timer);
        }

        StartTimer(&update_video_timer);
        updateVideo();
        update_video_time += GetTimer(update_video_timer);

        StartTimer(&update_audio_timer);
        updateAudio();
        update_audio_time += GetTimer(update_audio_timer);

        StartTimer(&recvfrom_timer);
        int recv_size = recvfrom(socketContext.s, &packet, sizeof(packet), 0, (struct sockaddr*)(&socketContext.addr), &slen);
        recvfrom_time += GetTimer(recvfrom_timer);

        int packet_size = sizeof(packet) - sizeof(packet.data) + packet.payload_size;
        total_recvs++;

        if (recv_size == 0) {
            // ACK
        }
        else if (recv_size < 0) {
            #if defined(_WIN32)
              int error = WSAGetLastError();
            #else
              int error = errno;
            #endif

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
            StartTimer(&hash_timer);
            uint32_t hash = 0;// Hash((char*)&packet + sizeof(packet.hash), packet_size - sizeof(packet.hash));
            hash_time += GetTimer(hash_timer);

            if (hash == packet.hash) {
                mprintf("Incorrect Hash\n");
            }
            else {
                //mprintf("\nRecv Time: %f\nRecvs: %d\nRecv Size: %d\nType: ", recv_time, total_recvs, recv_size);
                switch (packet.type) {
                case PACKET_VIDEO:
                    //mprintf("Recv Video %d %d\n", packet.id, packet.index);
                    StartTimer(&video_timer);
                    ReceiveVideo(&packet, recv_size);
                    video_time += GetTimer(video_timer);
                    max_video_time = max(max_video_time, GetTimer(video_timer));
                    break;
                case PACKET_AUDIO:
                    StartTimer(&audio_timer);
                    ReceiveAudio(&packet, recv_size);
                    audio_time += GetTimer(audio_timer);
                    max_audio_time = max(max_audio_time, GetTimer(audio_timer));
                    break;
                case PACKET_MESSAGE:
                    StartTimer(&message_timer); 
                    ReceiveMessage(&packet, recv_size);
                    message_time += GetTimer(message_timer);
                    break;
                default:
                    mprintf("Unknown Packet\n");
                    break;
                }
            }
        }
    }

    SDL_Delay(5);
}

static int32_t ReceiveMessage(struct RTPPacket* packet, int recv_size) {
    FractalServerMessage fmsg = *(FractalServerMessage*)packet->data;
    switch (fmsg.type) {
    case MESSAGE_PONG:
        if (ping_id == fmsg.ping_id) {
            //mprintf("Latency: %f\n", GetTimer(latency_timer));
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
    initMultiThreadedPrintf(false);

    for (int try_amount = 0; try_amount < 3; try_amount++) {
        send_packet_mutex = SDL_CreateMutex();

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

        if (CreateUDPContext(&PacketSendContext, "C", SERVER_IP, 10, 250) < 0) {
            exit(1);
        }

        struct SocketContext PacketReceiveContext = { 0 };
        if (CreateUDPContext(&PacketReceiveContext, "C", SERVER_IP, 1, 250) < 0) {
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

        clock ack_timer;
        SendAck(&PacketReceiveContext, 1);
        SDL_LockMutex(send_packet_mutex);
        SendAck(&PacketSendContext, 1);
        SDL_UnlockMutex(send_packet_mutex);
        StartTimer(&ack_timer);

        bool shutting_down = false;
        bool connected = true;
        while (connected && !shutting_down)
        {
            if (GetTimer(ack_timer) * 1000.0 > ACK_REFRESH_MS) {
                SendAck(&PacketReceiveContext, 1);
                SDL_LockMutex(send_packet_mutex);
                if (SendAck(&PacketSendContext, 1) < 0) {
                    mprintf("Could not send ACK!\n");
                }
                SDL_UnlockMutex(send_packet_mutex);
                StartTimer(&ack_timer);
            }

            if (needs_dimension_update && !tried_to_update_dimension && (server_width != OUTPUT_WIDTH || server_height != OUTPUT_HEIGHT)) {
                memset(&fmsg, 0, sizeof(fmsg));
                fmsg.type = MESSAGE_DIMENSIONS;
                fmsg.dimensions.width = OUTPUT_WIDTH;
                fmsg.dimensions.height = OUTPUT_HEIGHT;
                SendPacket(&fmsg, sizeof(fmsg));
                tried_to_update_dimension = true;
            }

            if (update_mbps) {
                //mprintf("Updating MBPS: %f\n", max_mbps);
                update_mbps = false;
                memset(&fmsg, 0, sizeof(fmsg));
                fmsg.type = MESSAGE_MBPS;
                fmsg.mbps = max_mbps;
                SendPacket(&fmsg, sizeof(fmsg));
            }

            if (is_timing_latency && GetTimer(latency_timer) > 0.5) {
                mprintf("Ping received no response: %d\n", ping_id);
                is_timing_latency = false;
                ping_failures++;
                if (ping_failures == 3) {
                    mprintf("Server disconnected: 3 consecutive ping failures.\n");
                    connected = false;
                }
            }

            if (!is_timing_latency && GetTimer(latency_timer) > 0.5) {
                memset(&fmsg, 0, sizeof(fmsg));
                ping_id++;
                fmsg.type = MESSAGE_PING;
                fmsg.ping_id = ping_id;
                is_timing_latency = true;

                StartTimer(&latency_timer);

                //mprintf("Ping! %d\n", ping_id);
                SendPacket(&fmsg, sizeof(fmsg));
                SendPacket(&fmsg, sizeof(fmsg));
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
                    SendPacket(&fmsg, sizeof(fmsg));
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

        if (shutting_down) {
            break;
        }
    }

    destroyMultiThreadedPrintf();
    printf("Closing Client...\n");

    return 0;
}
