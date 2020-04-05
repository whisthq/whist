#include "main.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/aes.h"
#include "../include/clipboard.h"
#include "../include/fractal.h"
#include "audio.h"
#include "video.h"

int audio_frequency = -1;

// Width and Height
volatile int server_width = -1;
volatile int server_height = -1;

// maximum mbps
volatile double max_mbps = MAXIMUM_MBPS;
volatile bool update_mbps = false;

// Global state variables
volatile SDL_Window* window;
volatile SDL_Renderer* renderer;
volatile bool run_receive_packets;
volatile bool is_timing_latency;
volatile clock latency_timer;
volatile int ping_id;
volatile int ping_failures;

volatile int output_width;
volatile int output_height;
volatile char* server_ip;

// Function Declarations

SDL_mutex* send_packet_mutex;
int ReceivePackets(void* opaque);
int ReceiveMessage(struct RTPPacket* packet);

struct SocketContext PacketSendContext;
struct SocketContext PacketTCPContext;

volatile bool connected = true;
volatile bool exiting = false;
volatile int try_amount;

// UPDATER CODE - HANDLES ALL PERIODIC UPDATES
struct UpdateData {
    bool needs_dimension_update;
    bool tried_to_update_dimension;
    bool has_initialized_updated;
    clock last_tcp_check_timer;
} volatile UpdateData;

void initUpdate() {
    UpdateData.needs_dimension_update = true;
    UpdateData.tried_to_update_dimension = false;

    StartTimer((clock*)&UpdateData.last_tcp_check_timer);
    StartTimer((clock*)&latency_timer);
    ping_id = 1;
    ping_failures = -2;

    initUpdateClipboard((SEND_FMSG*)&SendFmsg, server_ip);
    ClearReadingTCP();
}

void destroyUpdate() { destroyUpdateClipboard(); }

void update() {
    FractalClientMessage fmsg;

    if (GetTimer(UpdateData.last_tcp_check_timer) > 25.0 / 1000.0) {
        // Check if TCP is up
        int result = sendp(&PacketTCPContext, NULL, 0);
        if (result < 0) {
            mprintf("Lost TCP Connection (Error: %d)\n", GetLastNetworkError());
        }

        char* tcp_buf = TryReadingTCPPacket(&PacketTCPContext);
        if (tcp_buf) {
            struct RTPPacket* packet = (struct RTPPacket*)tcp_buf;
            struct FractalServerMessage* fmsg_response =
                (struct FractalServerMessage*)packet->data;
            mprintf("Received %d byte clipboard message from server!\n",
                    packet->payload_size);
            SetClipboard(&fmsg_response->clipboard);
        }
        StartTimer((clock*)&UpdateData.last_tcp_check_timer);
    }

    if (pendingUpdateClipboard()) {
        updateClipboard();
    }

    // Check if clipboard has updated
    if (hasClipboardUpdated()) {
        mprintf("Clipboard event found, sending to server!\n");
        updateClipboard();
    }

    // Start update checks
    if (UpdateData.needs_dimension_update &&
        !UpdateData.tried_to_update_dimension &&
        (server_width != output_width || server_height != output_height)) {
        mprintf("Asking for server dimension to be %dx%d\n", output_width,
                output_height);
        fmsg.type = MESSAGE_DIMENSIONS;
        fmsg.dimensions.width = output_width;
        fmsg.dimensions.height = output_height;
        SendFmsg(&fmsg);
        UpdateData.tried_to_update_dimension = true;
    }

    if (update_mbps) {
        mprintf("Asking for server MBPS to be %f\n", max_mbps);
        update_mbps = false;
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_mbps;
        SendFmsg(&fmsg);
    }
    // End update checks

    // Start Ping
    if (GetTimer(latency_timer) > 1.0) {
        mprintf("Whoah, ping timer is way too old\n");
    }

    if (is_timing_latency && GetTimer(latency_timer) > 0.6) {
        mprintf("Ping received no response: %d\n", ping_id);
        is_timing_latency = false;
        ping_failures++;
        if (ping_failures == 3) {
            mprintf("Server disconnected: 3 consecutive ping failures.\n");
            connected = false;
        }
    }

    static int num_ping_tries = 0;
    bool taking_a_bit = is_timing_latency &&
                        GetTimer(latency_timer) > 0.21 * (1 + num_ping_tries);
    bool awhile_since_last_ping =
        !is_timing_latency && GetTimer(latency_timer) > 0.5;

    if (awhile_since_last_ping || taking_a_bit) {
        if (is_timing_latency) {
            num_ping_tries++;
        } else {
            ping_id++;
            is_timing_latency = true;
            StartTimer((clock*)&latency_timer);
            num_ping_tries = 0;
        }

        fmsg.type = MESSAGE_PING;
        fmsg.ping_id = ping_id;

        mprintf("Ping! %d\n", ping_id);
        SendFmsg(&fmsg);
    }
    // End Ping
}
// END UPDATER CODE

int SendTCPPacket(void* data, int len);
int SendPacket(void* data, int len);

int SendFmsg(struct FractalClientMessage* fmsg) {
    if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return SendTCPPacket(fmsg, GetFmsgSize(fmsg));
    } else {
        return SendPacket(fmsg, GetFmsgSize(fmsg));
    }
}

#define LARGEST_PACKET 10000000
char unbounded_packet[LARGEST_PACKET];
char encrypted_unbounded_packet[sizeof(int) + LARGEST_PACKET + 16];

int SendTCPPacket(void* data, int len) {
    if (len > LARGEST_PACKET) {
        mprintf("Packet too large!\n");
        return -1;
    }

    struct RTPPacket* packet = (struct RTPPacket*)unbounded_packet;

    packet->id = -1;

    packet->type = PACKET_MESSAGE;
    memcpy(packet->data, data, len);
    packet->payload_size = len;

    int packet_size = PACKET_HEADER_SIZE + len;

    int encrypt_len = encrypt_packet(
        packet, packet_size,
        (struct RTPPacket*)(sizeof(int) + encrypted_unbounded_packet),
        (unsigned char*)PRIVATE_KEY);
    *((int*)encrypted_unbounded_packet) = encrypt_len;

    mprintf("Sending TCP Packet... %d\n", encrypt_len);
    bool failed = false;
    if (sendp(&PacketTCPContext, encrypted_unbounded_packet,
              sizeof(int) + encrypt_len) < 0) {
        mprintf("Failed to send packet!\n");
        failed = true;
    }
    mprintf("Successfully sent!\n");

    return failed ? -1 : 0;
}

static int sent_packet_id = 1;
int SendPacket(void* data, int len) {
    if (len > MAX_PAYLOAD_SIZE) {
        mprintf("Packet too large!\n");
        return -1;
    }

    struct RTPPacket packet = {0};

    packet.id = sent_packet_id;
    sent_packet_id++;

    packet.type = PACKET_MESSAGE;
    memcpy(packet.data, data, len);
    packet.payload_size = len;

    int packet_size = PACKET_HEADER_SIZE + len;

    struct RTPPacket encrypted_packet;
    int encrypt_len = encrypt_packet(&packet, packet_size, &encrypted_packet,
                                     (unsigned char*)PRIVATE_KEY);

    bool failed = false;
    SDL_LockMutex(send_packet_mutex);
    if (sendp(&PacketSendContext, &encrypted_packet, encrypt_len) < 0) {
        mprintf("Failed to send packet!\n");
        failed = true;
    }
    SDL_UnlockMutex(send_packet_mutex);

    return failed ? -1 : 0;
}

int ReceivePackets(void* opaque) {
    mprintf("ReceivePackets running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    struct RTPPacket packet = {0};
    struct SocketContext socketContext = *(struct SocketContext*)opaque;

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
    clock video_timer;
    clock audio_timer;
    clock message_timer;

    /****
    End Timers
    ****/

    initUpdate();

    double lastrecv = 0.0;

    clock last_ack;
    StartTimer(&last_ack);

    clock drop_test_timer;
    int drop_time_ms = 250;
    int drop_distance_sec = -1;
    bool dropping = false;
    StartTimer(&drop_test_timer);

    while (run_receive_packets) {
        if (GetTimer(last_ack) > 5.0) {
            sendp(&socketContext, NULL, 0);
            StartTimer(&last_ack);
        }

        update();

        // mprintf("Update\n");
        // Call as often as possible
        if (GetTimer(world_timer) > 5) {
            mprintf("\n");
            mprintf("world_time: %f\n", GetTimer(world_timer));
            mprintf("recvfrom_time: %f\n", recvfrom_time);
            mprintf("update_video_time: %f\n", update_video_time);
            mprintf("update_audio_time: %f\n", update_audio_time);
            mprintf("hash_time: %f\n", hash_time);
            mprintf("video_time: %f\n", video_time);
            mprintf("max_video_time: %f\n", max_video_time);
            mprintf("audio_time: %f\n", audio_time);
            mprintf("max_audio_time: %f\n", max_audio_time);
            mprintf("message_time: %f\n", message_time);
            mprintf("\n");
            StartTimer(&world_timer);
        }

        StartTimer(&update_video_timer);
        updateVideo();
        update_video_time += GetTimer(update_video_timer);

        StartTimer(&update_audio_timer);
        updateAudio();
        update_audio_time += GetTimer(update_audio_timer);

        StartTimer(&recvfrom_timer);

        // START DROP EMULATION
        if (dropping) {
            if (drop_time_ms > 0 &&
                GetTimer(drop_test_timer) * 1000.0 > drop_time_ms) {
                dropping = false;
                StartTimer(&drop_test_timer);
            }
        } else {
            if (drop_distance_sec > 0 &&
                GetTimer(drop_test_timer) > drop_distance_sec) {
                dropping = true;
                StartTimer(&drop_test_timer);
            }
        }
        // END DROP EMULATION

        int recv_size;
        if (dropping) {
            SDL_Delay(1);
            recv_size = 0;
            mprintf("DROPPING\n");
        } else {
            struct RTPPacket encrypted_packet;
            int encrypted_len = recvp(&socketContext, &encrypted_packet,
                                      sizeof(encrypted_packet));

            recv_size = encrypted_len;

            if (recv_size > 0) {
                recv_size =
                    decrypt_packet(&encrypted_packet, encrypted_len, &packet,
                                   (unsigned char*)PRIVATE_KEY);
                if (recv_size < 0) {
                    mprintf("Failed to decrypt packet\n");
                    // Just pretend like it never happened
                    recv_size = 0;
                }
            }
        }

        double recvfrom_short_time = GetTimer(recvfrom_timer);

        recvfrom_time += recvfrom_short_time;
        lastrecv += recvfrom_short_time;

        if (recv_size > 0) {
            if (lastrecv > 20.0 / 1000.0) {
                mprintf(
                    "Took more than 20ms to receive something!! Took %fms "
                    "total!\n",
                    lastrecv * 1000.0);
            }
            lastrecv = 0.0;
        }

        // mprintf("Recv wait time: %f\n", GetTimer(recvfrom_timer));

        total_recvs++;

        if (recv_size == 0) {
            // ACK
        } else if (recv_size < 0) {
            int error = GetLastNetworkError();

            switch (error) {
                case ETIMEDOUT:
                case EWOULDBLOCK:
                    break;
                default:
                    mprintf("Unexpected Packet Error: %d\n", error);
                    break;
            }
        } else {
            // mprintf("\nRecv Time: %f\nRecvs: %d\nRecv Size: %d\nType: ",
            // recv_time, total_recvs, recv_size);
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

    if (lastrecv > 20.0 / 1000.0) {
        mprintf("Took more than 20ms to receive something!! Took %fms total!\n",
                lastrecv * 1000.0);
    }

    SDL_Delay(5);

    destroyUpdate();

    return 0;
}

int ReceiveMessage(struct RTPPacket* packet) {
    if (packet->payload_size != sizeof(FractalServerMessage)) {
        mprintf("Incorrect payload size for a server message!\n");
    }
    FractalServerMessage fmsg = *(FractalServerMessage*)packet->data;
    switch (fmsg.type) {
        case SMESSAGE_CLIPBOARD:
            mprintf("Receive clipboard message from server!\n");
            SetClipboard(&fmsg.clipboard);
            break;
        case MESSAGE_PONG:
            if (ping_id == fmsg.ping_id) {
                mprintf("Latency: %f\n", GetTimer(latency_timer));
                is_timing_latency = false;
                ping_failures = 0;
                try_amount = 0;
            } else {
                mprintf("Old Ping ID found.\n");
            }
            break;
        case MESSAGE_AUDIO_FREQUENCY:
            mprintf("Changing audio frequency to %d\n", fmsg.frequency);
            audio_frequency = fmsg.frequency;
            break;
        case SMESSAGE_QUIT:
            mprintf("Server signaled a quit!\n");
            exiting = true;
            break;
        default:
            mprintf("Unknown Server Message Received\n");
            return -1;
    }
    return 0;
}

void clearSDL() {
    SDL_SetRenderDrawColor((SDL_Renderer*)renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear((SDL_Renderer*)renderer);
    SDL_RenderPresent((SDL_Renderer*)renderer);
}

// Make the screen black
void loadingSDL() {
    static SDL_Texture* wallpaper_screen_texture = NULL;
    static SDL_Texture* loading_screen_texture = NULL;
    if (!wallpaper_screen_texture) {
        SDL_Surface* loading_screen = SDL_LoadBMP("wallpaper.bmp");
        wallpaper_screen_texture = SDL_CreateTextureFromSurface(
            (SDL_Renderer*)renderer, loading_screen);
        SDL_FreeSurface(loading_screen);
    }
    if (!loading_screen_texture) {
        SDL_Surface* loading_screen = SDL_LoadBMP("loading_screen.bmp");
        loading_screen_texture = SDL_CreateTextureFromSurface(
            (SDL_Renderer*)renderer, loading_screen);
        SDL_FreeSurface(loading_screen);
    }

    /*
    SDL_SetRenderDrawColor( (SDL_Renderer*)renderer, 0, 0, 0, SDL_ALPHA_OPAQUE
    ); SDL_RenderClear( (SDL_Renderer*)renderer );
    */

    int w, h;
    SDL_Rect dstrect;

    SDL_QueryTexture(wallpaper_screen_texture, NULL, NULL, &w, &h);
    dstrect.x = output_width / 2 - w / 2;
    dstrect.y = output_height / 2 - h / 2;
    dstrect.w = w;
    dstrect.h = h;
    SDL_RenderCopy((SDL_Renderer*)renderer, wallpaper_screen_texture, NULL,
                   NULL);

    SDL_QueryTexture(loading_screen_texture, NULL, NULL, &w, &h);
    dstrect.x = output_width / 2 - w / 2;
    dstrect.y = output_height / 2 - h / 2;
    dstrect.w = w;
    dstrect.h = h;
    SDL_RenderCopy((SDL_Renderer*)renderer, loading_screen_texture, NULL,
                   &dstrect);

    SDL_RenderPresent((SDL_Renderer*)renderer);
}

// Send a key to SDL event queue, presumably one that is captured and wouldn't
// naturally make it to the event queue by itself
void SendCapturedKey(FractalKeycode key, int type, int time) {
    SDL_Event e = {0};
    e.type = type;
    e.key.keysym.scancode = (SDL_Scancode)key;
    e.key.timestamp = time;
    SDL_PushEvent(&e);
}

#if defined(_WIN32)
// Function to capture keyboard strokes and block them if they encode special
// key combinations, with intent to redirect them to SendCapturedKey so that the
// keys can still be streamed over to the host

HHOOK mule;
HHOOK g_hKeyboardHook;
BOOL g_bFullscreen;
LRESULT CALLBACK LowLevelKeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam) {
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;

    switch (nCode) {
        case HC_ACTION: {
            // Check to see if the CTRL key is pressed
            BOOL bControlKeyDown =
                GetAsyncKeyState(VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);
            BOOL bAltKeyDown = pkbhs->flags & LLKHF_ALTDOWN;

            int type = (pkbhs->flags & LLKHF_UP) ? SDL_KEYUP : SDL_KEYDOWN;
            int time = pkbhs->time;

            // Disable LWIN
            if (pkbhs->vkCode == VK_LWIN) {
                SendCapturedKey(FK_LGUI, type, time);
                return 1;
            }

            // Disable RWIN
            if (pkbhs->vkCode == VK_RWIN) {
                SendCapturedKey(FK_RGUI, type, time);
                return 1;
            }

            // Disable CTRL+ESC
            if (pkbhs->vkCode == VK_ESCAPE && bControlKeyDown) {
                SendCapturedKey(FK_ESCAPE, type, time);
                return 1;
            }

            // Disable ALT+ESC
            if (pkbhs->vkCode == VK_ESCAPE && bAltKeyDown) {
                SendCapturedKey(FK_ESCAPE, type, time);
                return 1;
            }

            // Disable ALT+TAB
            if (pkbhs->vkCode == VK_TAB && bAltKeyDown) {
                SendCapturedKey(FK_TAB, type, time);
                return 1;
            }

            // Disable ALT+F4
            if (pkbhs->vkCode == VK_F4 && bAltKeyDown) {
                SendCapturedKey(FK_F4, type, time);
                return 1;
            }

            break;
        }
        default:
            break;
    }
    return CallNextHookEx(mule, nCode, wParam, lParam);
}
#endif

int initSDL() {
#if defined(_WIN32)
    // Hook onto windows keyboard to intercept windows special key combinations
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                       GetModuleHandle(NULL), 0);
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    int full_width = get_native_screen_width();
    int full_height = get_native_screen_height();

    if (output_width < 0) {
        output_width = full_width;
    }

    if (output_height < 0) {
        output_height = full_height;
    }

    bool is_fullscreen =
        full_width == output_width && full_height == output_height;

#if defined(_WIN32)
    window = SDL_CreateWindow(
        "Fractal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, output_width,
        output_height,
        (is_fullscreen ? SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP : 0));
#else
    window =
        SDL_CreateWindow("Fractal", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, output_width, output_height,
                         (is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP |
                                              SDL_WINDOW_ALWAYS_ON_TOP
                                        : 0));
#endif

    if (!window) {
        fprintf(stderr, "SDL: could not create window - exiting: %s\n",
                SDL_GetError());
        return -1;
    }

    renderer =
        SDL_CreateRenderer((SDL_Window*)window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting: %s\n",
                SDL_GetError());
        return -1;
    }

    return 0;
}

void destroySDL() {
#if defined(_WIN32)
    UnhookWindowsHookEx(g_hKeyboardHook);
#endif

    if (renderer) {
        SDL_DestroyRenderer((SDL_Renderer*)renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow((SDL_Window*)window);
        window = NULL;
    }
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    initBacktraceHandler();
#ifndef _WIN32
    runcmd( "chmod 600 sshkey" );
#endif

    int num_required_args = 2;
    int num_optional_args = 3;
    if (argc - 1 < num_required_args ||
        argc - 1 > num_required_args + num_optional_args) {
        printf(
            "Usage: desktop [IP ADDRESS] [AES PRIVATE KEY] [[OPTIONAL] WIDTH] "
            "[[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]");
        return -1;
    }

    server_ip = argv[1];
    //    char* aes_private_key = argv[2]; TODO: need to make webserver exchange
    //    key instead of hardcode
    output_width = -1;
    output_height = -1;

    if (argc >= 4 && (atoi(argv[3]) > 0)) {
        output_width = atoi(argv[3]);
    }

    if (argc >= 5 && (atoi(argv[4]) > 0)) {
        output_height = atoi(argv[4]);
    }

    if (argc == 6) {
        max_mbps = atoi(argv[5]);
    }

    if (initSDL() < 0) {
        printf("Failed to initialized SDL\n");
        return -1;
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    initMultiThreadedPrintf(true);

    exiting = false;
    for (try_amount = 0; try_amount < 3 && !exiting; try_amount++) {
        // Make the screen black
        loadingSDL();

        // If this is a retry, wait a bit more for the server to recover
        if (try_amount > 0) {
            mprintf("Trying to recover the server connection...\n");
            SDL_Delay(1000);
        }

        // initialize the windows socket library if this is a windows client
#if defined(_WIN32)
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            mprintf("Failed to initialize Winsock with error code: %d.\n",
                    WSAGetLastError());
            return -1;
        }
#endif

        SDL_Event msg;
        FractalClientMessage fmsg = {0};

        if (CreateUDPContext(&PacketSendContext, "C", (char*)server_ip,
                             PORT_CLIENT_TO_SERVER, 10, 500) < 0) {
            mprintf("Failed to connect to server\n");
            continue;
        }

        SDL_Delay(150);

        struct SocketContext PacketReceiveContext = {0};
        if (CreateUDPContext(&PacketReceiveContext, "C", (char*)server_ip,
                             PORT_SERVER_TO_CLIENT, 1, 500) < 0) {
            mprintf("Failed finish connection to server\n");
            closesocket(PacketSendContext.s);
            continue;
        }

        SDL_Delay(150);

        if (CreateTCPContext(&PacketTCPContext, "C", (char*)server_ip,
                             PORT_SHARED_TCP, 1, 500) < 0) {
            mprintf("Failed finish connection to server\n");
            closesocket(PacketSendContext.s);
            closesocket(PacketReceiveContext.s);
            continue;
        }

        // Initialize video and audio
        send_packet_mutex = SDL_CreateMutex();
        is_timing_latency = false;
        connected = true;
        initVideo();
        initAudio();

        // Create thread to receive all packets and handle them as needed
        run_receive_packets = true;
        SDL_Thread* receive_packets_thread;
        receive_packets_thread = SDL_CreateThread(
            ReceivePackets, "ReceivePackets", &PacketReceiveContext);

        clock keyboard_sync_timer;
        StartTimer(&keyboard_sync_timer);

        // Initialize variables
        bool alt_pressed = false;
        bool ctrl_pressed = false;
        bool lgui_pressed = false;
        bool rgui_pressed = false;

        SDL_Delay(250);

        while (connected && !exiting) {
            // Update the keyboard state
            if (GetTimer(keyboard_sync_timer) > 50.0 / 1000.0) {
                SDL_Delay(5);
                fmsg.type = MESSAGE_KEYBOARD_STATE;

                int num_keys;
                Uint8* state = (Uint8*)SDL_GetKeyboardState(&num_keys);
#if defined(_WIN32)
                fmsg.num_keycodes = (short)min(NUM_KEYCODES, num_keys);
#else
                fmsg.num_keycodes = fmin(NUM_KEYCODES, num_keys);
#endif

                state[FK_LGUI] = lgui_pressed;
                state[FK_RGUI] = rgui_pressed;
                memcpy(fmsg.keyboard_state, state, fmsg.num_keycodes);

                fmsg.caps_lock = SDL_GetModState() & KMOD_CAPS;
                fmsg.num_lock = SDL_GetModState() & KMOD_NUM;

                SendFmsg(&fmsg);

                StartTimer(&keyboard_sync_timer);
            }

            fmsg.type = 0;
            if (SDL_PollEvent(&msg)) {
                switch (msg.type) {
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                        // Send a keyboard press for this event
                        fmsg.type = MESSAGE_KEYBOARD;
                        fmsg.keyboard.code =
                            (FractalKeycode)msg.key.keysym.scancode;
                        fmsg.keyboard.mod = msg.key.keysym.mod;
                        fmsg.keyboard.pressed = msg.key.type == SDL_KEYDOWN;

                        if (fmsg.keyboard.code == FK_LALT) {
                            alt_pressed = fmsg.keyboard.pressed;
                        }
                        if (fmsg.keyboard.code == FK_LCTRL) {
                            ctrl_pressed = fmsg.keyboard.pressed;
                        }
                        if (fmsg.keyboard.code == FK_LGUI) {
                            lgui_pressed = fmsg.keyboard.pressed;
                        }
                        if (fmsg.keyboard.code == FK_RGUI) {
                            rgui_pressed = fmsg.keyboard.pressed;
                        }

                        if (ctrl_pressed && alt_pressed &&
                            fmsg.keyboard.code == FK_F4) {
                            mprintf("Quitting...\n");
                            exiting = true;
                        }

                        break;
                    case SDL_MOUSEMOTION:
                        fmsg.type = MESSAGE_MOUSE_MOTION;
                        fmsg.mouseMotion.relative = SDL_GetRelativeMouseMode();
                        fmsg.mouseMotion.x =
                            fmsg.mouseMotion.relative
                                ? msg.motion.xrel
                                : msg.motion.x * 1000000 / output_width;
                        fmsg.mouseMotion.y =
                            fmsg.mouseMotion.relative
                                ? msg.motion.yrel
                                : msg.motion.y * 1000000 / output_height;
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        fmsg.type = MESSAGE_MOUSE_BUTTON;
                        fmsg.mouseButton.button = msg.button.button;
                        fmsg.mouseButton.pressed =
                            msg.button.type == SDL_MOUSEBUTTONDOWN;
                        break;
                    case SDL_MOUSEWHEEL:
                        fmsg.type = MESSAGE_MOUSE_WHEEL;
                        fmsg.mouseWheel.x = msg.wheel.x;
                        fmsg.mouseWheel.y = msg.wheel.y;
                        break;
                    case SDL_QUIT:
                        mprintf("Forcefully Quitting...\n");
                        exiting = true;
                        break;
                }

                if (fmsg.type != 0) {
                    SendFmsg(&fmsg);
                } else {
                    SDL_Delay(1);
                }
            }
        }

        mprintf("Disconnecting...\n");

        if (exiting) {
            fmsg.type = CMESSAGE_QUIT;
            SDL_Delay(50);
            SendFmsg(&fmsg);
            SDL_Delay(50);
            SendFmsg(&fmsg);
            SDL_Delay(50);
            SendFmsg(&fmsg);
            SDL_Delay(50);
        }

        run_receive_packets = false;
        SDL_WaitThread(receive_packets_thread, NULL);

        clearSDL();

        // Destroy video and audio
        destroyVideo();
        destroyAudio();

        closesocket(PacketSendContext.s);
        closesocket(PacketReceiveContext.s);
        closesocket(PacketTCPContext.s);

#if defined(_WIN32)
        WSACleanup();
#endif
    }

    mprintf("Closing Client...\n");

    destroyMultiThreadedPrintf();
    destroySDL();

    return 0;
}
