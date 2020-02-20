#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../include/fractal.h"
#include "../include/webserver.h" // header file for webserver query functions
#include "../include/linkedlist.h" // header file for audio decoder
#include "../include/aes.h"

#include "video.h"
#include "audio.h"

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
volatile bool run_receive_packets = false;
volatile bool is_timing_latency = false;
volatile clock latency_timer;
volatile int ping_id;
volatile int ping_failures;

volatile int output_width;
volatile int output_height;

// Function Declarations

SDL_mutex* send_packet_mutex;
int SendPacket(void* data, int len, bool encrypt);
static int32_t ReceivePackets(void* opaque);
static int32_t ReceiveMessage(struct RTPPacket* packet);

struct SocketContext PacketSendContext;

volatile bool connected = true;

// UPDATER CODE - HANDLES ALL PERIODIC UPDATES
struct UpdateData {
    bool needs_dimension_update;
    bool tried_to_update_dimension;
    bool has_initialized_updated;
} UpdateData;

void initUpdate() {
    UpdateData.needs_dimension_update = true;
    UpdateData.tried_to_update_dimension = false;

    StartTimer((clock*)&latency_timer);
    ping_id = 1;
    ping_failures = -2;
}

void update() {
    FractalClientMessage fmsg;

    // Start update checks
    if (UpdateData.needs_dimension_update && !UpdateData.tried_to_update_dimension && (server_width != output_width || server_height != output_height)) {
        mprintf("Asking for server dimension to be %dx%d\n", output_width, output_height);
        memset(&fmsg, 0, sizeof(fmsg));
        fmsg.type = MESSAGE_DIMENSIONS;
        fmsg.dimensions.width = output_width;
        fmsg.dimensions.height = output_height;
        SendPacket(&fmsg, sizeof(fmsg), true);
        UpdateData.tried_to_update_dimension = true;
    }

    if (update_mbps) {
        mprintf("Asking for server MBPS to be %f\n", max_mbps);
        update_mbps = false;
        memset(&fmsg, 0, sizeof(fmsg));
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_mbps;
        SendPacket(&fmsg, sizeof(fmsg), false);
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

    if (!is_timing_latency && GetTimer(latency_timer) > 0.5) {
        memset(&fmsg, 0, sizeof(fmsg));
        ping_id++;
        fmsg.type = MESSAGE_PING;
        fmsg.ping_id = ping_id;
        is_timing_latency = true;

        StartTimer((clock*)&latency_timer);

        mprintf("Ping! %d\n", ping_id);
        SendPacket(&fmsg, sizeof(fmsg), false);
        SendPacket(&fmsg, sizeof(fmsg), false);
    }
    // End Ping
}
// END UPDATER CODE

int SendPacket(void* data, int len, bool encrypt) {
    encrypt = false;

    if (len > MAX_PACKET_SIZE) {
        mprintf("Packet too large!\n");
        return -1;
    }

    bool failed = false;

    struct RTPPacket packet = { 0 };
    memcpy( packet.data, data, len );
    packet.payload_size = len;

    int packet_size = sizeof( packet ) - sizeof( packet.data ) + len;

    if( encrypt )
    {
        struct RTPPacket encrypted_packet;
        int encrypt_len = encrypt_packet( &packet, packet_size, &encrypted_packet, PRIVATE_KEY );
        memcpy( &packet, &encrypted_packet, encrypt_len );
        packet_size = encrypt_len;
    } else
    {
        packet.cipher_len = -1;
    }

    SDL_LockMutex(send_packet_mutex);
    if (sendp(&PacketSendContext, &packet, packet_size ) < 0) {
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

    initUpdate();

    double lastrecv = 0.0;

    clock last_ack;
    StartTimer(&last_ack);

    clock drop_test_timer;
    int drop_time_ms = 250;
    int drop_distance_sec = -1;
    bool dropping = false;
    StartTimer(&drop_test_timer);

    for (int i = 0; run_receive_packets; i++) {
        if (GetTimer(last_ack) > 5.0) {
            sendp(&socketContext, NULL, 0);
            StartTimer(&last_ack);
        }

        update();

        //mprintf("Update\n");
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
            if (drop_time_ms > 0 && GetTimer(drop_test_timer) * 1000.0 > drop_time_ms) {
                dropping = false;
                StartTimer(&drop_test_timer);
            }
        }
        else {
            if (drop_distance_sec > 0 && GetTimer(drop_test_timer) > drop_distance_sec) {
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
        }
        else {
            struct RTPPacket encrypted_packet;
            int encrypted_len = recvp(&socketContext, &encrypted_packet, sizeof(encrypted_packet));

            recv_size = encrypted_len;

            if( recv_size > 0 )
            {
                if( encrypted_packet.cipher_len == -1 )
                {
                    memcpy( &packet, &encrypted_packet, recv_size );
                } else
                {
                    mprintf( "Decryping\n" );
                    recv_size = decrypt_packet( &encrypted_packet, encrypted_len, &packet, PRIVATE_KEY );
                }
            }
        }

        double recvfrom_short_time = GetTimer(recvfrom_timer);

        recvfrom_time += recvfrom_short_time;
        lastrecv += recvfrom_short_time;

        if (recv_size > 0) {
            if (lastrecv > 20.0 / 1000.0) {
                mprintf("Took more than 20ms to receive something!! Took %fms total!\n", lastrecv * 1000.0);
            }
            lastrecv = 0.0;
        }

        //mprintf("Recv wait time: %f\n", GetTimer(recvfrom_timer));

        int packet_size = sizeof(packet) - sizeof(packet.data) + packet.payload_size;
        total_recvs++;

        if (recv_size == 0) {
            // ACK
        }
        else if (recv_size < 0) {
            int error = GetLastNetworkError();

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

    if (lastrecv > 20.0 / 1000.0) {
        mprintf("Took more than 20ms to receive something!! Took %fms total!\n", lastrecv * 1000.0);
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
            mprintf("Latency: %f\n", GetTimer(latency_timer));
            is_timing_latency = false;
            ping_failures = 0;
        }
        else {
            mprintf("Old Ping ID found.\n");
        }
        break;
    case MESSAGE_AUDIO_FREQUENCY:
        mprintf("Changing audio frequency to %d\n", fmsg.frequency);
        audio_frequency = fmsg.frequency;
        break;
    default:
        mprintf("Unknown Server Message Received\n");
        return -1; 
    }
    return 0;
}

void clearSDL() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void SendCapturedKey( FractalKeycode key, int type, int time)
{
    SDL_Event e = { 0 };
    e.type = type;
    e.key.keysym.scancode = key;
    e.key.timestamp = time;
    SDL_PushEvent( &e );
}

#if defined(_WIN32)
HHOOK mule;
HHOOK g_hKeyboardHook;
BOOL g_bFullscreen;
LRESULT CALLBACK LowLevelKeyboardProc( INT nCode, WPARAM wParam, LPARAM lParam )
{
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;

    switch( nCode )
    {
    case HC_ACTION:
    {
        // Check to see if the CTRL key is pressed
        BOOL bControlKeyDown = GetAsyncKeyState( VK_CONTROL ) >> ((sizeof( SHORT ) * 8) - 1);
        BOOL bAltKeyDown = pkbhs->flags & LLKHF_ALTDOWN;

        int type = (pkbhs->flags & LLKHF_UP) ? SDL_KEYUP : SDL_KEYDOWN;
        int time = pkbhs->time;

        // Disable WIN
        if( pkbhs->vkCode == VK_LWIN || pkbhs->vkCode == VK_RWIN )
        {
            SendCapturedKey( KEY_LGUI, type, time );
            return 1;
        }

        // Disable CTRL+ESC
        if( pkbhs->vkCode == VK_ESCAPE && bControlKeyDown )
        {
            SendCapturedKey( KEY_ESCAPE, type, time );
            return 1;
        }

        // Disable ALT+ESC
        if( pkbhs->vkCode == VK_ESCAPE && bAltKeyDown )
        {
            SendCapturedKey( KEY_ESCAPE, type, time );
            return 1;
        }

        // Disable ALT+TAB
        if( pkbhs->vkCode == VK_TAB && bAltKeyDown )
        {
            SendCapturedKey( KEY_TAB, type, time );
            return 1;
        }

        // Disable ALT+F4
        if( pkbhs->vkCode == VK_F4 && bAltKeyDown )
        {
            SendCapturedKey( KEY_F4, type, time );
            return 1;
        }

        break;
    }
    default:
        break;
    }
    return CallNextHookEx( mule, nCode, wParam, lParam );
}
#endif

int initSDL() {
#if defined(_WIN32)
    g_hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle( NULL ), 0 );
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

    bool is_fullscreen = full_width == output_width && full_height == output_height;

    window = SDL_CreateWindow(
        "Fractal",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        output_width,
        output_height,
        (is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
    );

    if (!window) {
        fprintf(stderr, "SDL: could not create window - exiting: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

void destroySDL() {

#if defined(_WIN32)
    UnhookWindowsHookEx( g_hKeyboardHook );
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

int main(int argc, char* argv[])
{
    int num_required_args = 2;
    int num_optional_args = 3;
    if(argc - 1 < num_required_args || argc - 1 > num_required_args + num_optional_args ) {
        printf("Usage: desktop [IP ADDRESS] [AES PRIVATE KEY] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]");
        return -1;
    }

    char* server_ip = argv[1];
    char* aes_private_key = argv[2];
    output_width = -1;
    output_height = -1;

    if (argc >= 4) {
        output_width = atoi(argv[3]);
    }

    if (argc >= 5) {
        output_height = atoi(argv[4]);
    }

    if (argc == 6) {
        max_mbps = atoi(argv[5]);
    }

    // START AES
    /*
    unsigned char* key = (unsigned char*)"0123456789012345";

    struct RTPPacket test_packet;
    struct RTPPacket encrypted_packet;
    struct RTPPacket decrypted_packet;

    printf( "Original Hash: %d\n", Hash( (char*)&test_packet + 300, 200 ) );

    int encrypt_len = encrypt_packet( &test_packet, 600, &encrypted_packet, key);

    printf( "Encrypted Hash: %d\n", Hash( &encrypted_packet, sizeof( test_packet ) ) );

    int decrypt_len;
    decrypt_len = decrypt_packet( &encrypted_packet, encrypt_len, &decrypted_packet, key );
    printf( "Decrypt len: %d\n", decrypt_len );
    printf( "Decrypted Hash: %d\n", Hash( (char*)&decrypted_packet + 300, 200 ) );

    return 0;
    unsigned char* iv = (unsigned char*)"0123456789012345";

    unsigned char* plaintext =
        (unsigned char*)"The quick brown fox jumps over the lazy dog";

    unsigned char ciphertext[128];
    unsigned char decryptedtext[128];

    int decryptedtext_len, ciphertext_len;

    ciphertext_len = encrypt( plaintext, strlen( (char*)plaintext ), key, iv,
                              ciphertext );
    printf( "Ciphertext is:\n" );
    decryptedtext_len = decrypt( ciphertext, ciphertext_len, key, iv,
                                 decryptedtext );
    decryptedtext[decryptedtext_len] = '\0';
    printf( "Decrypted text is:\n" );
    printf( "%s\n", decryptedtext );
    */

    // END AES

    if (initSDL() < 0) {
        printf("Failed to initialized SDL\n");
        return -1;
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
    initMultiThreadedPrintf(false);

    for (int try_amount = 0; try_amount < 3; try_amount++) {
        clearSDL();

        SDL_Delay(200);

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

        if (CreateUDPContext(&PacketSendContext, "C", server_ip, PORT_CLIENT_TO_SERVER, 10, 500) < 0) {
            mprintf("Failed to connect to server\n");
            continue;
        }

        SDL_Delay(150);

        struct SocketContext PacketReceiveContext = { 0 };
        if (CreateUDPContext(&PacketReceiveContext, "C", server_ip, PORT_SERVER_TO_CLIENT, 1, 500) < 0) {
            mprintf("Failed finish connection to server\n");
            closesocket(PacketSendContext.s);
            continue;
        }

        // Initialize variables
        connected = true;
        bool shutting_down = false;
        bool alt_pressed = false;
        bool ctrl_pressed = false;

        // Initialize video and audio
        initVideo();
        initAudio();

        // Create thread to receive all packets and handle them as needed
        send_packet_mutex = SDL_CreateMutex();
        run_receive_packets = true;
        SDL_Thread* receive_packets_thread;
        receive_packets_thread = SDL_CreateThread(ReceivePackets, "ReceivePackets", &PacketReceiveContext);

        while (connected && !shutting_down)
        {
            memset(&fmsg, 0, sizeof(fmsg));
            if (SDL_PollEvent(&msg)) {
                switch (msg.type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    fmsg.type = MESSAGE_KEYBOARD;
                    fmsg.keyboard.code = (FractalKeycode)msg.key.keysym.scancode;
                    fmsg.keyboard.mod = msg.key.keysym.mod;
                    fmsg.keyboard.pressed = msg.key.type == SDL_KEYDOWN;

                    if (fmsg.keyboard.code == KEY_LALT) {
                        alt_pressed = fmsg.keyboard.pressed;
                    }
                    if (fmsg.keyboard.code == KEY_LCTRL) {
                        ctrl_pressed = fmsg.keyboard.pressed;
                    }
                    if (fmsg.keyboard.code == KEY_F4) {
                        if (alt_pressed && ctrl_pressed) {
                            mprintf("Quitting...\n");
                            shutting_down = true;
                        }
                    }

                    break;
                case SDL_MOUSEMOTION:
                    fmsg.type                 = MESSAGE_MOUSE_MOTION;
                    fmsg.mouseMotion.relative = SDL_GetRelativeMouseMode();
                    fmsg.mouseMotion.x        = fmsg.mouseMotion.relative ? msg.motion.xrel : msg.motion.x * 1000000 / output_width;
                    fmsg.mouseMotion.y        = fmsg.mouseMotion.relative ? msg.motion.yrel : msg.motion.y * 1000000 / output_height;
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
                case SDL_CLIPBOARDUPDATE:
                    mprintf("Clipboard!\n");
                    break;
                case SDL_QUIT:
                    mprintf("Forcefully Quitting...\n");
                    shutting_down = true;
                    break;
                }

                if (fmsg.type != 0) {
                    SendPacket(&fmsg, sizeof(fmsg), true);
                }
                else {
                    SDL_Delay(1);
                }
            }
        }

        mprintf("Disconnecting...\n");

        run_receive_packets = false;
        SDL_WaitThread(receive_packets_thread, NULL);

        // Destroy video and audio
        destroyVideo();
        destroyAudio();

        clearSDL();

        closesocket(PacketSendContext.s);
        closesocket(PacketReceiveContext.s);
#if defined(_WIN32)
        WSACleanup();
#endif

        if (shutting_down) {
            break;
        }

        SDL_Delay(750);
    }

    mprintf("Closing Client...\n");

    destroyMultiThreadedPrintf();
    destroySDL();

    return 0;
}
