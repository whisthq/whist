/*
 * Fractal Client.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "main.h"

#include <errno.h>
#include <limits.h>
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
#include "sdl_utils.h"
#include "video.h"

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
volatile bool is_spectator = false;
volatile int connection_id;
volatile SDL_Window* window;
volatile bool run_receive_packets;
volatile bool is_timing_latency;
volatile clock latency_timer;
volatile int ping_id;
volatile int ping_failures;

volatile int output_width;
volatile int output_height;
volatile CodecType codec_type;
volatile char* server_ip;

// Function Declarations

int ReceivePackets(void* opaque);
int ReceiveMessage(FractalPacket* packet);
bool received_server_init_message;

SocketContext PacketSendContext;
SocketContext PacketTCPContext;

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

    initClipboardSynchronizer((char*)server_ip);
}

void destroyUpdate() { destroyClipboardSynchronizer(); }

/**
 * Check all pending updates, and act on those pending updates
 * to actually update the state of our programs
 * This function expects to be called at minimum every 5ms to keep the program
 * up-to-date
 *
 * @return void
 */
void update() {
    FractalClientMessage fmsg;

    // Check for a new clipboard update from the server, if it's been 25ms since
    // the last time we checked the TCP socket, and the clipboard isn't actively
    // busy
    if (GetTimer(UpdateData.last_tcp_check_timer) > 25.0 / 1000.0 &&
        !isClipboardSynchronizing() && !is_spectator) {
        // Check if TCP connction is active
        int result = Ack(&PacketTCPContext);
        if (result < 0) {
            LOG_ERROR("Lost TCP Connection (Error: %d)", GetLastNetworkError());
            // TODO: Should exit or recover protocol if TCP connection is lost
        }

        // Receive tcp buffer, if a full packet has been received
        FractalPacket* tcp_packet = ReadTCPPacket(&PacketTCPContext);
        if (tcp_packet) {
            ReceiveMessage(tcp_packet);
        }

        // Update the last tcp check timer
        StartTimer((clock*)&UpdateData.last_tcp_check_timer);
    }

    // Assuming we have all of the important init information, then update the
    // clipboard
    if (received_server_init_message) {
        ClipboardData* clipboard = ClipboardSynchronizerGetNewClipboard();
        if (clipboard) {
            FractalClientMessage* fmsg_clipboard =
                malloc(sizeof(FractalClientMessage) + sizeof(ClipboardData) +
                       clipboard->size);
            fmsg_clipboard->type = CMESSAGE_CLIPBOARD;
            memcpy(&fmsg_clipboard->clipboard, clipboard,
                   sizeof(ClipboardData) + clipboard->size);
            SendFmsg(fmsg_clipboard);
            free(fmsg_clipboard);
        }
    }

    // If we haven't yet tried to update the dimension, and the dimensions don't
    // line up, then request the proper dimension
    if (!UpdateData.tried_to_update_dimension &&
        (server_width != output_width || server_height != output_height)) {
        LOG_INFO("Asking for server dimension to be %dx%d", output_width,
                 output_height);
        fmsg.type = MESSAGE_DIMENSIONS;
        fmsg.dimensions.width = (int)output_width;
        fmsg.dimensions.height = (int)output_height;
        fmsg.dimensions.codec_type = (CodecType)codec_type;
        fmsg.dimensions.dpi =
            (int)(96.0 * output_width / get_virtual_screen_width());
        SendFmsg(&fmsg);
        UpdateData.tried_to_update_dimension = true;
    }

    // If the code has triggered a mbps update, then notify the server of the
    // newly desired mbps
    if (update_mbps) {
        update_mbps = false;
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_bitrate / 1024.0 / 1024.0;
        LOG_INFO("Asking for server MBPS to be %f", fmsg.mbps);
        SendFmsg(&fmsg);
    }

    // If it's been 1 second since the last ping, we should warn
    if (GetTimer(latency_timer) > 1.0) {
        LOG_WARNING("Whoah, ping timer is way too old");
    }

    // If we're waiting for a ping, and it's been 600ms, then that ping will be
    // noted as failed
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
    // If 500ms has past since the last resolved ping, then it's been a while
    // and we should ping again (Last resolved ping is a ping that either has
    // been received, or was noted as failed)
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

// Large fmsg's should be sent over TCP. At the moment, this is only CLIPBOARD
// messages FractalClientMessage packet over UDP that requires multiple
// sub-packets to send, it not supported (If low latency large
// FractalClientMessage packets are needed, then this will have to be
// implemented)
int SendFmsg(FractalClientMessage* fmsg) {
    if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return SendTCPPacket(&PacketTCPContext, PACKET_MESSAGE, fmsg,
                             GetFmsgSize(fmsg));
    } else {
        static int sent_packet_id = 0;
        sent_packet_id++;
        return SendUDPPacket(&PacketSendContext, PACKET_MESSAGE, fmsg,
                             GetFmsgSize(fmsg), sent_packet_id, -1, NULL, NULL);
    }
}

int ReceivePackets(void* opaque) {
    LOG_INFO("ReceivePackets running on Thread %d", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    SocketContext socketContext = *(SocketContext*)opaque;

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
    // This code will drop packets intentionally to test protocol's ability to
    // handle such events drop_distance_sec is the number of seconds in-between
    // simulated drops drop_time_ms is how long the drop will last for
    clock drop_test_timer;
    int drop_time_ms = 250;
    int drop_distance_sec = -1;
    bool is_currently_dropping = false;
    StartTimer(&drop_test_timer);

    // Initialize update handler
    initUpdate();

    while (run_receive_packets) {
        if (GetTimer(last_ack) > 5.0) {
            Ack(&socketContext);
            StartTimer(&last_ack);
        }

        // Handle all pending updates
        update();

        // TODO hash_time is never updated, leaving it in here in case it is
        // used in the future
        // casting to suppress warnings.
        (void)hash_time;
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
        // We will do it here, after receiving each packet or if the last recvp
        // timed out

        StartTimer(&update_video_timer);
        updateVideo();
        update_video_time += GetTimer(update_video_timer);

        StartTimer(&update_audio_timer);
        updateAudio();
        update_audio_time += GetTimer(update_audio_timer);

        // Time the following recvfrom code
        StartTimer(&recvfrom_timer);

        // START DROP EMULATION
        if (is_currently_dropping) {
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

        FractalPacket* packet;

        if (is_currently_dropping) {
            // Simulate dropping packets but just not calling recvp
            SDL_Delay(1);
            LOG_INFO("DROPPING");
            packet = NULL;
        } else {
            packet = ReadUDPPacket(&socketContext);
        }

        double recvfrom_short_time = GetTimer(recvfrom_timer);

        // Total amount of time spent in recvfrom / decrypt_packet
        recvfrom_time += recvfrom_short_time;
        // Total amount of cumulative time spend in recvfrom, since the last
        // time recv_size was > 0
        lastrecv += recvfrom_short_time;

        if (packet) {
            // Log if it's been a while since the last packet was received
            if (lastrecv > 20.0 / 1000.0) {
                LOG_INFO(
                    "Took more than 20ms to receive something!! Took %fms "
                    "total!",
                    lastrecv * 1000.0);
            }
            lastrecv = 0.0;
        }

        // LOG_INFO("Recv wait time: %f", GetTimer(recvfrom_timer));

        if (packet) {
            // Check packet type and then redirect packet to the proper packet
            // handler
            switch (packet->type) {
                case PACKET_VIDEO:
                    // Video packet
                    StartTimer(&video_timer);
                    ReceiveVideo(packet);
                    video_time += GetTimer(video_timer);
                    max_video_time = max(max_video_time, GetTimer(video_timer));
                    break;
                case PACKET_AUDIO:
                    // Audio packet
                    StartTimer(&audio_timer);
                    ReceiveAudio(packet);
                    audio_time += GetTimer(audio_timer);
                    max_audio_time = max(max_audio_time, GetTimer(audio_timer));
                    break;
                case PACKET_MESSAGE:
                    // A FractalServerMessage for other information
                    StartTimer(&message_timer);
                    ReceiveMessage(packet);
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

// Receiving a FractalServerMessage
int ReceiveMessage(FractalPacket* packet) {
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
                LOG_INFO("Old Ping ID found: Got %d but expected %d",
                         fmsg->ping_id, ping_id);
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
            ClipboardSynchronizerSetClipboard(&fmsg->clipboard);
            break;
        case MESSAGE_INIT:
            // Receiving a bunch of initializing server data for a new
            // connection
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

            FILE* f = fopen(path, "w");
            fprintf(f, "%d", msg_init->connection_id);
            fclose(f);

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

int parseArgs(int argc, char* argv[]) {
    char* usage =
        "Usage: desktop [IP ADDRESS] [[OPTIONAL] WIDTH]"
        " [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE] [[OPTIONAL] SPECTATE]\n";
    int num_required_args = 1;
    int num_optional_args = 5;
    if (argc - 1 < num_required_args ||
        argc - 1 > num_required_args + num_optional_args) {
        printf("%s", usage);
        return -1;
    }

    server_ip = argv[1];

    output_width = 0;
    output_height = 0;

    long int ret;
    char* endptr;

    if (argc >= 3) {
        errno = 0;
        ret = strtol(argv[2], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
            printf("%s", usage);
            return -1;
        }
        if (ret != 0) output_width = (int)ret;
    }

    if (argc >= 4) {
        errno = 0;
        ret = strtol(argv[3], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
            printf("%s", usage);
            return -1;
        }
        if (ret != 0) output_height = (int)ret;
    }

    if (argc == 5) {
        errno = 0;
        ret = strtol(argv[4], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret <= 0) {
            printf("%s", usage);
            return -1;
        }
        max_bitrate = (int)ret;
    }

    is_spectator = false;
    if (argc == 6) {
        is_spectator = true;
    }

    return 0;
}

int main(int argc, char* argv[]) {
#ifndef _WIN32
    runcmd("chmod 600 sshkey", NULL);
    // files can't be written to a macos app bundle, so they need to be
    // cached in /Users/USERNAME/.APPNAME, here .fractal directory
    // attempt to create fractal cache directory, it will fail if it
    // already exists, which is fine
    // for Linux, this is in /home/USERNAME/.fractal, the cache is also needed
    // for the same reason
    runcmd("mkdir ~/.fractal", NULL);
    runcmd("chmod 0755 ~/.fractal", NULL);

    // the mkdir command won't do anything if the folder already exists, in
    // which case we make sure to clear the previous logs and connection id
    runcmd("rm -f ~/.fractal/log.txt", NULL);
    runcmd("rm -f ~/.fractal/connection_id.txt", NULL);
#endif

    int ret = parseArgs(argc, argv);
    if (ret != 0) return ret;

    // Write ecdsa key to a local file for ssh to use, for that server ip
    // This will identify the connecting server as the correct server and not an
    // imposter
    FILE* ssh_key_host = fopen("ssh_host_ecdsa_key.pub", "w");
    fprintf(ssh_key_host, "%s %s\n", server_ip, HOST_PUBLIC_KEY);
    fclose(ssh_key_host);

    // Initialize the SDL window
    window = initSDL(output_width, output_height);
    if (!window) {
        LOG_ERROR("Failed to initialized SDL");
        return -1;
    }

    // After creating the window, we will grab DPI-adjusted dimensions in real
    // pixels
    output_width = get_window_pixel_width((SDL_Window*)window);
    output_height = get_window_pixel_height((SDL_Window*)window);

    // Set all threads to highest priority
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

#ifdef _WIN32
    // no cache needed on windows, use local directory
    initLogger(".");
#else  // macos, linux
    // apple cache, can't be in the same folder as bundled app
    // this stores log.txt in Users/USERNAME/.fractal/log.txt
    char path[100] = "";
    strcat(path, getenv("HOME"));
    strcat(path, "/.fractal");
    initLogger(path);
#endif

    PrintSystemInfo();

    // Initialize clipboard and video
    initVideo();
    exiting = false;

    int tcp_connection_timeout = 250;

    // Try 3 times if a failure to connect occurs
    for (try_amount = 0; try_amount < 3 && !exiting; try_amount++) {
        // If this is a retry, wait a bit more for the server to recover
        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
            SDL_Delay(1000);
        }

#if defined(_WIN32)
        // Initialize the windows socket library if this is a windows client
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            mprintf("Failed to initialize Winsock with error code: %d.\n",
                    WSAGetLastError());
            return -1;
        }
#endif

        // Create connection contexts to the server application

        // First context: Sending packets to server

        bool using_stun = true;
        SocketContext PacketReceiveContext = {0};

        if (is_spectator) {
            if (CreateUDPContext(&PacketReceiveContext, (char*)server_ip,
                                 PORT_SPECTATOR, 10, 500, true) < 0) {
                LOG_INFO(
                    "Server is not on STUN, attempting to connect directly");
                using_stun = false;
                if (CreateUDPContext(&PacketReceiveContext, (char*)server_ip,
                                     PORT_SPECTATOR, 10, 500, false) < 0) {
                    LOG_WARNING("Failed to connect to server");
                    continue;
                }
            }

            FractalPacket* init_spectator =
                ReadUDPPacket(&PacketReceiveContext);
            clock init_spectator_timer;
            StartTimer(&init_spectator_timer);
            while (!init_spectator && GetTimer(init_spectator_timer) < 1.0) {
                SDL_Delay(5);
                init_spectator = ReadUDPPacket(&PacketReceiveContext);
            }

            if (init_spectator) {
                FractalServerMessage* fmsg = init_spectator->data;
                LOG_INFO("SPECTATOR PORT: %d", fmsg->spectator_port);

                closesocket(PacketReceiveContext.s);

                if (CreateUDPContext(&PacketReceiveContext, (char*)server_ip,
                                     fmsg->spectator_port, 10, 500, true) < 0) {
                    LOG_INFO(
                        "Server is not on STUN, attempting to connect "
                        "directly");
                    using_stun = false;
                    if (CreateUDPContext(&PacketReceiveContext,
                                         (char*)server_ip, fmsg->spectator_port,
                                         10, 500, false) < 0) {
                        LOG_WARNING("Failed to connect to server");
                        continue;
                    }
                }

                PacketSendContext = PacketReceiveContext;
            } else {
                closesocket(PacketReceiveContext.s);
                LOG_WARNING("DID NOT RECEIVE SPECTATOR INIT FROM SERVER");
                continue;
            }
        } else {
            if (CreateUDPContext(&PacketSendContext, (char*)server_ip,
                                 PORT_CLIENT_TO_SERVER, 10, 500, true) < 0) {
                LOG_INFO(
                    "Server is not on STUN, attempting to connect directly");
                using_stun = false;
                if (CreateUDPContext(&PacketSendContext, (char*)server_ip,
                                     PORT_CLIENT_TO_SERVER, 10, 500,
                                     false) < 0) {
                    LOG_WARNING("Failed to connect to server");
                    continue;
                }
            }

            SDL_Delay(150);

            // Second context: Receiving packets from server

            if (CreateUDPContext(&PacketReceiveContext, (char*)server_ip,
                                 PORT_SERVER_TO_CLIENT, 1, 500,
                                 using_stun) < 0) {
                LOG_ERROR("Failed finish connection to server");
                closesocket(PacketSendContext.s);
                continue;
            }

            int a = 65535;
            if (setsockopt(PacketReceiveContext.s, SOL_SOCKET, SO_RCVBUF,
                           (const char*)&a, sizeof(int)) == -1) {
                fprintf(stderr, "Error setting socket opts: %s\n",
                        strerror(errno));
            }

            SDL_Delay(150);

            // Third context: Mutual TCP context for essential but
            // not-speed-sensitive applications

            if (CreateTCPContext(&PacketTCPContext, (char*)server_ip,
                                 PORT_SHARED_TCP, 1, tcp_connection_timeout,
                                 using_stun) < 0) {
                LOG_ERROR("Failed finish connection to server");
                tcp_connection_timeout += 250;
                closesocket(PacketSendContext.s);
                closesocket(PacketReceiveContext.s);
                continue;
            }
        }

        // Initialize audio and variables
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

        // Initialize keyboard state variables
        bool alt_pressed = false;
        bool ctrl_pressed = false;
        bool lgui_pressed = false;
        bool rgui_pressed = false;

        if (!is_spectator) {
            clock waiting_for_init_timer;
            StartTimer(&waiting_for_init_timer);
            while (!received_server_init_message) {
                // If 500ms and no init timer was received, we should disconnect
                // because something failed
                if (GetTimer(waiting_for_init_timer) > 500 / 1000.0) {
                    LOG_ERROR("Took too long for init timer!");
                    exiting = true;
                    break;
                }
                SDL_Delay(25);
            }
        }

        SDL_Event msg;
        FractalClientMessage fmsg = {0};

        clock ack_timer;
        StartTimer(&ack_timer);

        // Poll input for as long as we are connected and not exiting
        // This code will run once every millisecond
        while (connected && !exiting) {
            // Send acks to sockets every 5 seconds
            if (GetTimer(ack_timer) > 5) {
                Ack(&PacketSendContext);
                if (!is_spectator) {
                    Ack(&PacketTCPContext);
                }
                StartTimer(&ack_timer);
            }

            // Every 50ms we should syncronize the keyboard state
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

                // lgui/rgui don't work with SDL_GetKeyboardState for some
                // reason, so set manually
                state[FK_LGUI] = lgui_pressed;
                state[FK_RGUI] = rgui_pressed;
                // Copy keyboard state
                memcpy(fmsg.keyboard_state, state, fmsg.num_keycodes);

                // Also send caps lock and num lock status for syncronization
                fmsg.caps_lock = SDL_GetModState() & KMOD_CAPS;
                fmsg.num_lock = SDL_GetModState() & KMOD_NUM;

                SendFmsg(&fmsg);

                StartTimer(&keyboard_sync_timer);
            }

            // Check if event is waiting
            fmsg.type = 0;
            if (SDL_PollEvent(&msg)) {
                // Grab SDL event and handle the various input and window events
                switch (msg.type) {
                    case SDL_WINDOWEVENT:
                        if (msg.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                            // Let video thread know about the resizing to
                            // reinitialize display dimensions
                            output_width =
                                get_window_pixel_width((SDL_Window*)window);
                            output_height =
                                get_window_pixel_height((SDL_Window*)window);
                            set_video_active_resizing(false);

                            // Let the server know the new dimensions so that it
                            // can change native dimensions for monitor
                            fmsg.type = MESSAGE_DIMENSIONS;
                            fmsg.dimensions.width = output_width;
                            fmsg.dimensions.height = output_height;
                            fmsg.dimensions.codec_type = (CodecType)codec_type;
                            fmsg.dimensions.dpi =
                                (int)(96.0 * output_width /
                                      get_virtual_screen_width());

                            LOG_INFO(
                                "Window %d resized to %dx%d (Physical %dx%d)\n",
                                msg.window.windowID, msg.window.data1,
                                msg.window.data2, output_width, output_height);
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

                        // Keep memory of alt/ctrl/lgui/rgui status
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
                        // Relative motion is the delta x and delta y from last
                        // mouse position Absolute mouse position is where it is
                        // on the screen We multiply by scaling factor so that
                        // integer division doesn't destroy accuracy
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
                        // Handle mouse click
                        fmsg.type = MESSAGE_MOUSE_BUTTON;
                        // Record if left / right / middle button
                        fmsg.mouseButton.button = msg.button.button;
                        fmsg.mouseButton.pressed =
                            msg.button.type == SDL_MOUSEBUTTONDOWN;
                        break;
                    case SDL_MOUSEWHEEL:
                        // Record mousewheel x/y change
                        fmsg.type = MESSAGE_MOUSE_WHEEL;
                        fmsg.mouseWheel.x = msg.wheel.x;
                        fmsg.mouseWheel.y = msg.wheel.y;
                        break;
                    case SDL_QUIT:
                        // Quit if SDL_QUIT received
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

        // Send quit message to server so that it can efficiently disconnect
        // from the protocol and start accepting new connections
        if (exiting) {
            // Send a couple times just to more sure it gets sent
            fmsg.type = CMESSAGE_QUIT;
            SDL_Delay(50);
            SendFmsg(&fmsg);
            SDL_Delay(50);
            SendFmsg(&fmsg);
            SDL_Delay(50);
            SendFmsg(&fmsg);
            SDL_Delay(50);
        }

        // Trigger end of ReceivePackets thread
        run_receive_packets = false;
        SDL_WaitThread(receive_packets_thread, NULL);

        // Destroy audio
        destroyAudio();

        // Close all open sockets
        closesocket(PacketSendContext.s);
        closesocket(PacketReceiveContext.s);
        if (!is_spectator) {
            closesocket(PacketTCPContext.s);
        }

#if defined(_WIN32)
        WSACleanup();
#endif
    }

    // Destroy video, logger, and SDL

    destroyVideo();
    LOG_INFO("Closing Client...");

    destroySDL((SDL_Window*)window);
    destroyLogger();

    return 0;
}
