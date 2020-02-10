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
volatile SDL_Window* window;
volatile bool run_receive_packets = false;
volatile bool is_timing_latency = false;
volatile clock latency_timer;
volatile int ping_id;
volatile int ping_failures;

volatile int output_width;
volatile int output_height;

volatile int32_t positionX;
volatile int32_t positionY;

// Function Declarations

SDL_mutex* send_packet_mutex;
int SendPacket(void* data, int len);
static int32_t ReceivePackets(void* opaque);
static int32_t ReceiveMessage(struct RTPPacket* packet);

struct SocketContext PacketSendContext;


int SendPacket(void* data, int len) {
    if (len > MAX_PACKET_SIZE) {
        mprintf("Packet too large!\n");
        return -1;
    }

    bool failed = false;

    SDL_LockMutex(send_packet_mutex);
    if (sendp(&PacketSendContext, data, len) < 0) {
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
            mprintf("message_time: %f\n\n", message_time);
            StartTimer(&world_timer);
        }

        StartTimer(&update_video_timer);
        updateVideo();
        update_video_time += GetTimer(update_video_timer);

        StartTimer(&update_audio_timer);
        updateAudio();
        update_audio_time += GetTimer(update_audio_timer);

        StartTimer(&recvfrom_timer);
        int recv_size = recvp(&socketContext, &packet, sizeof(packet));
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
                    StartTimer(&video_timer);
                    ReceiveVideo(&packet);
                    video_time += GetTimer(video_timer);
                    max_video_time = max(max_video_time, GetTimer(video_timer));
                    break;
                case PACKET_AUDIO:
                    StartTimer(&audio_timer);
                    ReceiveAudio(&packet);
                    audio_time += GetTimer(audio_timer);
                    max_audio_time = max(max_audio_time, GetTimer(audio_timer));
                    break;
                case PACKET_MESSAGE:
                    StartTimer(&message_timer); 
                    ReceiveMessage(&packet);
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

    return 0;
}

static int32_t ReceiveMessage(struct RTPPacket* packet) {
    if (packet->payload_size != sizeof(FractalServerMessage)) {
        mprintf("Incorrect payload size for a server message!\n");
    }
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

int initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        mprintf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    int full_width = GetSystemMetrics(SM_CXSCREEN);
    int full_height = GetSystemMetrics(SM_CYSCREEN);

    bool is_fullscreen = full_width == output_width && full_height == output_height;

    window = SDL_CreateWindow(
        "Fractal",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        output_width,
        output_height,
        is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0
    );

    if (!window) {
        mprintf("SDL: could not create window - exiting\n");
        return -1;
    }

    return 0;
}

void destroySDL() {
    if (window) {
        SDL_DestroyWindow((SDL_Window*)window);
    }
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    if(argc < 2 || argc > 4) {
        printf("Usage: desktop [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT]");
        return -1;
    }

    char* server_ip = argv[1];
    output_width = GetSystemMetrics(SM_CXSCREEN);
    output_height = GetSystemMetrics(SM_CYSCREEN);

    if(argc >= 3) {
        output_width = atoi(argv[2]);
    }

    if(argc == 4) {
        output_height = atoi(argv[3]);
    }

    if (initSDL() < 0) {
        mprintf("Failed to initialized SDL\n");
        return -1;
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    initMultiThreadedPrintf(false);

    for (int try_amount = 0; try_amount < 3; try_amount++) {
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

        if (CreateUDPContext(&PacketSendContext, "C", server_ip, 10, 250) < 0) {
            mprintf("Failed to connect to server\n");
            continue;
        }

        struct SocketContext PacketReceiveContext = { 0 };
        if (CreateUDPContext(&PacketReceiveContext, "C", server_ip, 1, 250) < 0) {
            mprintf("Failed finish connection to server\n");
            continue;
        }

        // Initialize video and audio
        initVideo();
        initAudio();

        // Create thread to receive all packets and handle them as needed
        send_packet_mutex = SDL_CreateMutex();
        run_receive_packets = true;
        SDL_Thread* receive_packets_thread;
        receive_packets_thread = SDL_CreateThread(ReceivePackets, "ReceivePackets", &PacketReceiveContext);

        bool needs_dimension_update = true;
        bool tried_to_update_dimension = false;
        clock last_dimension_update;
        StartTimer(&last_dimension_update);

        bool shutting_down = false;
        bool connected = true;

        StartTimer((clock*)&latency_timer);
        ping_id = 1;
        ping_failures = -2;

        while (connected && !shutting_down)
        {
            if (needs_dimension_update && !tried_to_update_dimension && (server_width != output_width || server_height != output_height)) {
                memset(&fmsg, 0, sizeof(fmsg));
                fmsg.type = MESSAGE_DIMENSIONS;
                fmsg.dimensions.width = output_width;
                fmsg.dimensions.height = output_height;
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

                StartTimer((clock*)&latency_timer);

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
                    if (server_width > -1 && server_height > -1) {
                        fmsg.type = MESSAGE_MOUSE_MOTION;
                        fmsg.mouseMotion.x = msg.motion.x * 1000000 / output_width;
                        fmsg.mouseMotion.y = msg.motion.y * 1000000 / output_height;
                        positionX = msg.motion.x * server_width / output_width;
                        positionY = msg.motion.y * server_height / output_height;
                    }
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

        // Destroy video and audio
        destroyVideo();
        destroyAudio();

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

    mprintf("Closing Client...\n");

    destroyMultiThreadedPrintf();
    destroySDL();

    return 0;
}
