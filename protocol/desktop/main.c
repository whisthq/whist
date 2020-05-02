/*
 * Fractal Client.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "main.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fractal/clipboard/clipboard.h"
#include "../fractal/core/fractal.h"
#include "../fractal/network/network.h"
#include "../fractal/utils/aes.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "audio.h"
#include "video.h"
#include "sdl_utils.h"

#ifdef __APPLE__
#include "../fractal/utils/mac_utils.h"
#endif

int audio_frequency = -1;

// Width and Height
volatile int server_width = -1;
volatile int server_height = -1;

// maximum mbps
volatile int max_bitrate = STARTING_BITRATE;
volatile bool update_mbps = false;

// Global state variables
volatile int connection_id;
volatile SDL_Window* window;
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
bool received_server_init_message;

struct SocketContext PacketSendContext;
struct SocketContext PacketTCPContext;

volatile bool connected = true;
volatile bool exiting = false;
volatile int try_amount;

// Data
char filename[300];
char username[50];

// UPDATER CODE - HANDLES ALL PERIODIC UPDATES
struct UpdateData {
    bool tried_to_update_dimension;
    bool has_initialized_updated;
    clock last_tcp_check_timer;
} volatile UpdateData;

void initUpdate() {
    UpdateData.tried_to_update_dimension = false;

    StartTimer((clock*)&UpdateData.last_tcp_check_timer);
    StartTimer((clock*)&latency_timer);
    ping_id = 1;
    ping_failures = -2;

    initUpdateClipboard((SEND_FMSG*)&SendFmsg, (char*)server_ip);
    ClearReadingTCP();
}

void destroyUpdate() { destroyUpdateClipboard(); }

void update() {
    FractalClientMessage fmsg;

    // Check for clipboard update, if it's been 25ms since the last TCP check, and the clipboard isn't actively being updated
    if (GetTimer(UpdateData.last_tcp_check_timer) > 25.0 / 1000.0 &&
        !isUpdatingClipboard()) {

        // Check if TCP connction is active
        int result = sendp(&PacketTCPContext, NULL, 0);
        if (result < 0) {
            LOG_ERROR("Lost TCP Connection (Error: %d)",
                        GetLastNetworkError());
            // TODO: Should exit or recover protocol if TCP connection is lost
        }

        // Receive tcp buffer, if a full packet has been received
        char* tcp_buf = TryReadingTCPPacket(&PacketTCPContext);
        if (tcp_buf) {
            struct RTPPacket* packet = (struct RTPPacket*)tcp_buf;
            ReceiveMessage(packet);
        }

        // Update the last tcp check timer
        StartTimer((clock*)&UpdateData.last_tcp_check_timer);
    }

    // If the user has copied or cut something, then try pushing a clipboard update
    // NOTE: If clipboard is busy, then pendingUpdateClipboard will start returning true
    if (hasClipboardUpdated() && received_server_init_message) {
        LOG_INFO("Clipboard event found, sending to server!");
        updateClipboard();
    }

    // If we have a pendingUpdateClipboard, then try pushing that update
    // NOTE: We have to do this, because hasClipboardUpdated will only be true once
    // If the clipboard is busy while hasClipboardUpdated is true, then even when
    // hasClipboardUpdated becomes false, then pendingUpdateClipboard will remain
    // true until the clipboard update has been properly pushed
    if( pendingUpdateClipboard() && received_server_init_message )
    {
        updateClipboard();
    }

    // If we haven't yet tried to update the dimension, and the dimensions don't line up, then request the proper dimension
    if (!UpdateData.tried_to_update_dimension &&
        (server_width != output_width || server_height != output_height)) {
        LOG_INFO("Asking for server dimension to be %dx%d", output_width,
                 output_height);
        fmsg.type = MESSAGE_DIMENSIONS;
        fmsg.dimensions.width = output_width;
        fmsg.dimensions.height = output_height;
        fmsg.dimensions.dpi =
            (int)(96.0 * output_width / get_virtual_screen_width());
        SendFmsg(&fmsg);
        UpdateData.tried_to_update_dimension = true;
    }

    // If the code has triggered a mbps update, then notify the server of the newly desired mbps
    if (update_mbps) {
        LOG_INFO("Asking for server MBPS to be %f", max_bitrate);
        update_mbps = false;
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_bitrate / 1024.0 / 1024.0;
        SendFmsg(&fmsg);
    }

    // If it's been 1 second since the last ping, we should warn
    if (GetTimer(latency_timer) > 1.0) {
        LOG_WARNING("Whoah, ping timer is way too old");
    }

    // If we're waiting for a ping, and it's been 600ms, then that ping will be noted as failed
    if (is_timing_latency && GetTimer(latency_timer) > 0.6) {
        LOG_WARNING("Ping received no response: %d", ping_id);
        is_timing_latency = false;

        // Keep track of failures, and exit if too many failures
        ping_failures++;
        if (ping_failures == 3) {
            LOG_ERROR("Server disconnected: 3 consecutive ping failures.");
            connected = false;
        }
    }

    static int num_ping_tries = 0;

    // If 210ms has past since last ping, then it's taking a bit
    // Ie, a ping try will occur every 210ms
    bool taking_a_bit = is_timing_latency &&
                        GetTimer(latency_timer) > 0.21 * (1 + num_ping_tries);
    // If 500ms has past since the last resolved ping, then it's been a while and we should ping again
    // (Last resolved ping is a ping that either has been received, or was noted as failed)
    bool awhile_since_last_resolved_ping =
        !is_timing_latency && GetTimer(latency_timer) > 0.5;

    // If either of the two above conditions hold, then send a new ping
    if (awhile_since_last_resolved_ping || taking_a_bit) {
        if (is_timing_latency) {
            // A continuation of an existing ping that's been taking a bit
            num_ping_tries++;
        } else {
            // A brand new ping
            ping_id++;
            is_timing_latency = true;
            StartTimer((clock*)&latency_timer);
            num_ping_tries = 0;
        }

        fmsg.type = MESSAGE_PING;
        fmsg.ping_id = ping_id;

        LOG_INFO("Ping! %d", ping_id);
        SendFmsg(&fmsg);
    }
    // End Ping
}
// END UPDATER CODE

int SendTCPPacket(void* data, int len);
int SendPacket(void* data, int len);

// Large fmsg's should be sent over TCP. At the moment, this is only CLIPBOARD messages
// FractalClientMessage packet over UDP that requires multiple sub-packets to send, it not supported
// (If low latency large FractalClientMessage packets are needed, then this will have to be implemented)
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
    // Verify packet size can fit
    if (len > LARGEST_PACKET - PACKET_HEADER_SIZE ) {
        LOG_WARNING("Packet too large!");
        return -1;
    }

    struct RTPPacket* packet = (struct RTPPacket*)unbounded_packet;

    // Initialize the packet with all relevant data
    packet->id = -1;
    packet->type = PACKET_MESSAGE;
    memcpy(packet->data, data, len);
    packet->payload_size = len;

    // Calculate packet size
    int packet_size = PACKET_HEADER_SIZE + len;

    // Encrypt the packet using aes encryption
    int encrypt_len = encrypt_packet(
        packet, packet_size,
        (struct RTPPacket*)(sizeof(int) + encrypted_unbounded_packet),
        (unsigned char*)PRIVATE_KEY);
    *((int*)encrypted_unbounded_packet) = encrypt_len;

    // Send the packet
    LOG_INFO("Sending TCP Packet... %d\n", encrypt_len);
    bool failed = false;
    if (sendp(&PacketTCPContext, encrypted_unbounded_packet,
              sizeof(int) + encrypt_len) < 0) {
        LOG_WARNING("Failed to send packet!");
        failed = true;
    }
    LOG_INFO("Successfully sent!");

    // Return success code
    return failed ? -1 : 0;
}

int SendPacket(void* data, int len) {
    // Verify packet size can fit
    if (len > MAX_PAYLOAD_SIZE - PACKET_HEADER_SIZE ) {
        LOG_WARNING("Packet too large!");
        return -1;
    }

    // Local packet
    struct RTPPacket packet = {0};

    // Packet ID is ever incrementing
    static int sent_packet_id = 1;
    packet.id = sent_packet_id;
    sent_packet_id++;

    // Initialize packet data
    packet.type = PACKET_MESSAGE;
    memcpy(packet.data, data, len);
    packet.payload_size = len;

    int packet_size = PACKET_HEADER_SIZE + len;

    // Encrypt the packet using aes encryption
    struct RTPPacket encrypted_packet;
    int encrypt_len = encrypt_packet(&packet, packet_size, &encrypted_packet,
                                     (unsigned char*)PRIVATE_KEY);

    // Send the packet
    bool failed = false;
    SDL_LockMutex(send_packet_mutex);
    if (sendp(&PacketSendContext, &encrypted_packet, encrypt_len) < 0) {
        LOG_WARNING("Failed to send packet!");
        failed = true;
    }
    SDL_UnlockMutex(send_packet_mutex);

    // Return success code
    return failed ? -1 : 0;
}

int ReceivePackets(void* opaque) {
    LOG_INFO("ReceivePackets running on Thread %d", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    struct SocketContext socketContext = *(struct SocketContext*)opaque;
    struct RTPPacket packet = {0};

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

    double lastrecv = 0.0;

    clock last_ack;
    StartTimer(&last_ack);

    // NOTE: FOR DEBUGGING
    // This code will drop packets intentionally to test protocol's ability to handle such events
    // drop_distance_sec is the number of seconds in-between simulated drops
    // drop_time_ms is how long the drop will last for
    clock drop_test_timer;
    int drop_time_ms = 250;
    int drop_distance_sec = -1;
    bool is_currently_dropping = false;
    StartTimer(&drop_test_timer);

    // Initialize update handler
    initUpdate();

    while (run_receive_packets) {
        if (GetTimer(last_ack) > 5.0) {
            sendp(&socketContext, NULL, 0);
            StartTimer(&last_ack);
        }

        // Handle all pending updates
        update();

        // Post statistics every 5 seconds
        if (GetTimer(world_timer) > 5) {
            LOG_INFO("world_time: %f", GetTimer(world_timer));
            LOG_INFO("recvfrom_time: %f", recvfrom_time);
            LOG_INFO("update_video_time: %f", update_video_time);
            LOG_INFO("update_audio_time: %f", update_audio_time);
            LOG_INFO("hash_time: %f", hash_time);
            LOG_INFO("video_time: %f", video_time);
            LOG_INFO("max_video_time: %f", max_video_time);
            LOG_INFO("audio_time: %f", audio_time);
            LOG_INFO("max_audio_time: %f", max_audio_time);
            LOG_INFO("message_time: %f", message_time);
            StartTimer(&world_timer);
        }

        // Video and Audio should be updated at least every 5ms
        // We will do it here, after receiving each packet or if the last recvp timed out

        StartTimer(&update_video_timer);
        updateVideo();
        update_video_time += GetTimer(update_video_timer);

        StartTimer(&update_audio_timer);
        updateAudio();
        update_audio_time += GetTimer(update_audio_timer);

        // Time the following recvfrom code
        StartTimer(&recvfrom_timer);

        // START DROP EMULATION
        if ( is_currently_dropping ) {
            if (drop_time_ms > 0 &&
                GetTimer(drop_test_timer) * 1000.0 > drop_time_ms) {
                is_currently_dropping = false;
                StartTimer(&drop_test_timer);
            }
        } else {
            if (drop_distance_sec > 0 &&
                GetTimer(drop_test_timer) > drop_distance_sec) {
                is_currently_dropping = true;
                StartTimer(&drop_test_timer);
            }
        }
        // END DROP EMULATION

        int recv_size;
        if ( is_currently_dropping ) {
            // Simulate dropping packets but just not calling recvp
            SDL_Delay(1);
            recv_size = 0;
            LOG_INFO("DROPPING");
        } else {
            // Wait to receive packet over TCP, until timing out
            struct RTPPacket encrypted_packet;
            int encrypted_len = recvp(&socketContext, &encrypted_packet,
                                      sizeof(encrypted_packet));

            recv_size = encrypted_len;

            // If the packet was successfully received, then decrypt it
            if (recv_size > 0) {
                recv_size =
                    decrypt_packet(&encrypted_packet, encrypted_len, &packet,
                                   (unsigned char*)PRIVATE_KEY);

                // If there was an issue decrypting it, post warning and then ignore the problem
                if (recv_size < 0) {
                    LOG_WARNING("Failed to decrypt packet");
                    // Just pretend like it never happened
                    recv_size = 0;
                }
            }
        }

        double recvfrom_short_time = GetTimer(recvfrom_timer);

        // Total amount of time spent in recvfrom / decrypt_packet
        recvfrom_time += recvfrom_short_time;
        // Total amount of cumulative time spend in recvfrom, since the last time recv_size was > 0
        lastrecv += recvfrom_short_time;

        if (recv_size > 0) {
            // Log if it's been a while since the last packet was received
            if (lastrecv > 20.0 / 1000.0) {
                LOG_INFO(
                    "Took more than 20ms to receive something!! Took %fms "
                    "total!",
                    lastrecv * 1000.0);
            }
            lastrecv = 0.0;
        }

        LOG_INFO("Recv wait time: %f", GetTimer(recvfrom_timer));

        if (recv_size == 0) {
            // This packet was just an ACK; It can be ignored
        } else if (recv_size < 0) {
            // If the packet has an issue, and it wasn't just a simple timeout, then we should log it
            int error = GetLastNetworkError();

            switch (error) {
                case ETIMEDOUT:
                case EWOULDBLOCK:
                    break;
                default:
                    LOG_WARNING("Unexpected Packet Error: %d", error);
                    break;
            }
        } else {
            // Check packet type and then redirect packet to the proper packet handler
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
                    LOG_WARNING("Unknown Packet");
                    break;
            }
        }
    }

    if (lastrecv > 20.0 / 1000.0) {
        LOG_INFO("Took more than 20ms to receive something!! Took %fms total!",
                 lastrecv * 1000.0);
    }

    SDL_Delay(5);

    destroyUpdate();

    return 0;
}

int ReceiveMessage(struct RTPPacket* packet) {
    // Extract fmsg from the packet
    FractalServerMessage* fmsg = (FractalServerMessage*)packet->data;

    // Verify packet size
    if (fmsg->type == MESSAGE_INIT) {
        if (packet->payload_size !=
            sizeof(FractalServerMessage) + sizeof(FractalServerMessageInit)) {
            LOG_ERROR(
                "Incorrect payload size for a server message (type "
                "MESSAGE_INIT)!");
            return -1;
        }
    } else if (fmsg->type == SMESSAGE_CLIPBOARD) {
        if ((unsigned int)packet->payload_size !=
            sizeof(FractalServerMessage) + fmsg->clipboard.size) {
            LOG_ERROR(
                "Incorrect payload size for a server message (type "
                "SMESSAGE_CLIPBOARD)!");
            return -1;
        }
    } else {
        if (packet->payload_size != sizeof(FractalServerMessage)) {
            LOG_ERROR("Incorrect payload size for a server message! (type: %d)",
                      (int)packet->type);
            return -1;
        }
    }

    // Case on FractalServerMessage and handle each unique server message
    switch (fmsg->type) {
        case MESSAGE_PONG:
            // Received a response to our pings
            if (ping_id == fmsg->ping_id) {
                // Post latency since the last ping
                LOG_INFO("Latency: %f", GetTimer(latency_timer));
                is_timing_latency = false;
                ping_failures = 0;
                try_amount = 0;
            } else {
                LOG_INFO("Old Ping ID found: Got %d but expected %d", fmsg->ping_id, ping_id);
            }
            break;
        case MESSAGE_AUDIO_FREQUENCY:
            // Update audio frequency
            LOG_INFO("Changing audio frequency to %d", fmsg->frequency);
            audio_frequency = fmsg->frequency;
            break;
        case SMESSAGE_CLIPBOARD:
            // Receiving a clipboard update
            LOG_INFO("Received %d byte clipboard message from server!",
                     packet->payload_size);
            updateSetClipboard(&fmsg->clipboard);
            break;
        case MESSAGE_INIT:
            // Receiving a bunch of initializing server data for a new connection
            LOG_INFO("Received init message!\n");
            FractalServerMessageInit* msg_init =
                (FractalServerMessageInit*)fmsg->init_msg;
            memcpy(filename, msg_init->filename,
                   min(sizeof(filename), sizeof(msg_init->filename)));
            memcpy(username, msg_init->username,
                   min(sizeof(username), sizeof(msg_init->username)));

#ifdef _WIN32
            char* path = "connection_id.txt";
#else
            // mac apps can't save files into the bundled app package,
            // need to save into hidden folder under account
            // this stores connection_id in
            // Users/USERNAME/.fractal/connection_id.txt
            // same for Linux, but will be in
            // /home/USERNAME/.fractal/connection_id.txt
            char path[200] = "";
            strcat(path, getenv("HOME"));
            strcat(path, "/.fractal/connection_id.txt");

#endif

            FILE* f = fopen( path, "w" );
            fprintf( f, "%d", msg_init->connection_id );
            fclose( f );

            received_server_init_message = true;
            break;
        case SMESSAGE_QUIT:
            // Server wants to quit
            LOG_INFO("Server signaled a quit!");
            exiting = true;
            break;
        default:
            // Invalid FractalServerMessage type
            LOG_WARNING("Unknown FractalServerMessage Received");
            return -1;
    }
    return 0;
}

#define HOST_PUBLIC_KEY                                           \
    "ecdsa-sha2-nistp256 "                                        \
    "AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBOT1KV+" \
    "I511l9JilY9vqkp+QHsRve0ZwtGCBarDHRgRtrEARMR6sAPKrqGJzW/"     \
    "Zt86r9dOzEcfrhxa+MnVQhNE8="

int main(int argc, char* argv[]) {
#ifndef _WIN32
    runcmd("chmod 600 sshkey");
    // files can't be written to a macos app bundle, so they need to be
    // cached in /Users/USERNAME/.APPNAME, here .fractal directory
    // attempt to create fractal cache directory, it will fail if it
    // already exists, which is fine
    // for Linux, this is in /home/USERNAME/.fractal, the cache is also needed
    // for the same reason
    runcmd( "mkdir ~/.fractal" );
    runcmd( "chmod 0755 ~/.fractal" );
#endif
    initBacktraceHandler();

    int num_required_args = 1;
    int num_optional_args = 3;
    if (argc - 1 < num_required_args ||
        argc - 1 > num_required_args + num_optional_args) {
        printf(
            "Usage: desktop [IP ADDRESS] [[OPTIONAL] WIDTH] "
            "[[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]");
        return -1;
    }

    server_ip = argv[1];

    output_width = -1;
    output_height = -1;

    if (argc >= 3 && (atoi(argv[2]) > 0)) {
        output_width = atoi(argv[2]);
    }

    if (argc >= 4 && (atoi(argv[3]) > 0)) {
        output_height = atoi(argv[3]);
    }

    if (argc == 5) {
        max_bitrate = atoi(argv[4]);
    }

    FILE* ssh_key_host = fopen("ssh_host_ecdsa_key.pub", "w");
    fprintf(ssh_key_host, "%s %s\n", server_ip, HOST_PUBLIC_KEY);
    fclose(ssh_key_host);

    window = initSDL(output_width, output_height);
    if (!window) {
        LOG_ERROR("Failed to initialized SDL");
        return -1;
    }
    // After creating the window, we will grab DPI-adjusted dimensions in real pixels
    output_width = get_window_pixel_width( (SDL_Window*)window );
    output_height = get_window_pixel_height( (SDL_Window*)window );

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
#ifdef _WIN32
    // no cache needed on windows
    initLogger(".");
#else  // macos, linux
    // apple cache, can't be in the same folder as bundled app
    // this stores log.txt in Users/USERNAME/.fractal/log.txt
    char path[100] = "";
    strcat(path, getenv("HOME"));
    strcat(path, "/.fractal");
    initLogger(path);
#endif

    initClipboard();

    exiting = false;
    initVideo();

    for (try_amount = 0; try_amount < 3 && !exiting; try_amount++) {
        // If this is a retry, wait a bit more for the server to recover
        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
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

        if (CreateUDPContext(&PacketSendContext, ORIGIN_CLIENT,
                             (char*)server_ip, PORT_CLIENT_TO_SERVER, 10,
                             500) < 0) {
            LOG_WARNING("Failed to connect to server");
            continue;
        }

        SDL_Delay(150);

        struct SocketContext PacketReceiveContext = {0};
        if (CreateUDPContext(&PacketReceiveContext, ORIGIN_CLIENT,
                             (char*)server_ip, PORT_SERVER_TO_CLIENT, 1,
                             500) < 0) {
            LOG_ERROR("Failed finish connection to server");
            closesocket(PacketSendContext.s);
            continue;
        }

        SDL_Delay(150);

        if (CreateTCPContext(&PacketTCPContext, ORIGIN_CLIENT, (char*)server_ip,
                             PORT_SHARED_TCP, 1, 500) < 0) {
            LOG_ERROR("Failed finish connection to server");
            closesocket(PacketSendContext.s);
            closesocket(PacketReceiveContext.s);
            continue;
        }

        // Initialize video and audio
        send_packet_mutex = SDL_CreateMutex();
        is_timing_latency = false;
        connected = true;
        initAudio();

        // Create thread to receive all packets and handle them as needed
        run_receive_packets = true;
        SDL_Thread* receive_packets_thread;
        received_server_init_message = false;
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

        clock waiting_for_init_timer;
        StartTimer(&waiting_for_init_timer);
        while (!received_server_init_message) {
            if (GetTimer(waiting_for_init_timer) > 350 / 1000.0) {
                LOG_ERROR("Took too long for init timer!");
                exiting = true;
                break;
            }
            SDL_Delay(25);
        }

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
                    case SDL_WINDOWEVENT:
                        if( msg.window.event == SDL_WINDOWEVENT_SIZE_CHANGED )
                        {
                            set_video_active_resizing( false );
                            output_width = get_window_pixel_width( (SDL_Window*)window );
                            output_height = get_window_pixel_height( (SDL_Window*)window );

                            fmsg.type = MESSAGE_DIMENSIONS;
                            fmsg.dimensions.width = output_width;
                            fmsg.dimensions.height = output_height;
                            fmsg.dimensions.dpi =
                                (int)(96.0 * output_width / get_virtual_screen_width());

                            LOG_INFO( "Window %d resized to %dx%d (Physical %dx%d)\n", msg.window.windowID,
                                      msg.window.data1, msg.window.data2, output_width, output_height );
                        }
                        break;
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
                            LOG_INFO("Quitting...");
                            exiting = true;
                        }

                        break;
                    case SDL_MOUSEMOTION:
                        fmsg.type = MESSAGE_MOUSE_MOTION;
                        fmsg.mouseMotion.relative = SDL_GetRelativeMouseMode();
                        fmsg.mouseMotion.x = fmsg.mouseMotion.relative
                                                 ? msg.motion.xrel
                                                 : msg.motion.x *
                                                       MOUSE_SCALING_FACTOR /
                                                       get_window_virtual_width(
                                                           (SDL_Window*)window);
                        fmsg.mouseMotion.y =
                            fmsg.mouseMotion.relative
                                ? msg.motion.yrel
                                : msg.motion.y * MOUSE_SCALING_FACTOR /
                                      get_window_virtual_height(
                                          (SDL_Window*)window);
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
                        LOG_ERROR("Forcefully Quitting...");
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

        LOG_INFO("Disconnecting...");

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

        // Destroy video and audio
        destroyAudio();

        closesocket(PacketSendContext.s);
        closesocket(PacketReceiveContext.s);
        closesocket(PacketTCPContext.s);

#if defined(_WIN32)
        WSACleanup();
#endif
    }

    destroyVideo();
    LOG_INFO("Closing Client...");

    destroyLogger();
    destroySDL( (SDL_Window*)window );

    return 0;
}
