/*
 * Fractal Client.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "main.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fractal/clipboard/clipboard.h"
#include "../fractal/core/fractal.h"
#include "../fractal/network/network.h"
#include "../fractal/utils/aes.h"
#include "../fractal/utils/clock.h"
#include "../fractal/utils/logging.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "audio.h"
#include "desktop_utils.h"
#include "network.h"
#include "sdl_event_handler.h"
#include "sdl_utils.h"
#include "server_message_handler.h"
#include "video.h"

#ifdef __APPLE__
#include "../fractal/utils/mac_utils.h"
#endif

int audio_frequency = -1;

// Width and Height
volatile int server_width = -1;
volatile int server_height = -1;
volatile CodecType server_codec_type = CODEC_TYPE_UNKNOWN;

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
volatile CodecType output_codec_type = CODEC_TYPE_H264;
volatile char* server_ip;
int time_to_run_ci = 300;  // Seconds to run CI tests for
volatile int running_ci = 0;

int UDP_port = -1;
int TCP_port = -1;
int client_id = -1;
int uid;

// Keyboard state variables
bool alt_pressed = false;
bool ctrl_pressed = false;
bool lgui_pressed = false;
bool rgui_pressed = false;

clock window_resize_timer;

// Function Declarations

int ReceivePackets(void* opaque);
bool received_server_init_message;

SocketContext PacketSendContext = {0};
SocketContext PacketReceiveContext = {0};
SocketContext PacketTCPContext = {0};

volatile bool connected = true;
volatile bool exiting = false;
volatile int try_amount;

// Data
char filename[300];
char username[50];

#define MS_IN_SECOND 1000.0
#define WINDOWS_DEFAULT_DPI 96.0
#define BYTES_IN_KILOBYTE 1024.0

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
    if (GetTimer(UpdateData.last_tcp_check_timer) > 25.0 / MS_IN_SECOND && !isClipboardSynchronizing()) {
        // Check if TCP connction is active
        int result = Ack(&PacketTCPContext);
        if (result < 0) {
            LOG_ERROR("Lost TCP Connection (Error: %d)", GetLastNetworkError());
            // TODO: Should exit or recover protocol if TCP connection is lost
        }

        // Receive tcp buffer, if a full packet has been received
        FractalPacket* tcp_packet = ReadTCPPacket(&PacketTCPContext);
        if (tcp_packet) {
            handleServerMessage((FractalServerMessage*)tcp_packet->data,
                                (size_t)tcp_packet->payload_size);
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
                malloc(sizeof(FractalClientMessage) + sizeof(ClipboardData) + clipboard->size);
            fmsg_clipboard->type = CMESSAGE_CLIPBOARD;
            memcpy(&fmsg_clipboard->clipboard, clipboard, sizeof(ClipboardData) + clipboard->size);
            SendFmsg(fmsg_clipboard);
            free(fmsg_clipboard);
        }
    }

    // If we haven't yet tried to update the dimension, and the dimensions don't
    // line up, then request the proper dimension
    if (!UpdateData.tried_to_update_dimension &&
        (server_width != output_width || server_height != output_height ||
         server_codec_type != output_codec_type)) {
        LOG_INFO("Asking for server dimension to be %dx%d with codec type h%d", output_width,
                 output_height, output_codec_type);
        fmsg.type = MESSAGE_DIMENSIONS;
        fmsg.dimensions.width = (int)output_width;
        fmsg.dimensions.height = (int)output_height;
        fmsg.dimensions.codec_type = (CodecType)output_codec_type;
        fmsg.dimensions.dpi = (int)(WINDOWS_DEFAULT_DPI * output_width / get_virtual_screen_width());
        SendFmsg(&fmsg);
        UpdateData.tried_to_update_dimension = true;
    }

    // If the code has triggered a mbps update, then notify the server of the
    // newly desired mbps
    if (update_mbps) {
        update_mbps = false;
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_bitrate / BYTES_IN_KILOBYTE / BYTES_IN_KILOBYTE;
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
    bool taking_a_bit = is_timing_latency && GetTimer(latency_timer) > 0.21 * (1 + num_ping_tries);
    // If 500ms has past since the last resolved ping, then it's been a while
    // and we should ping again (Last resolved ping is a ping that either has
    // been received, or was noted as failed)
    bool awhile_since_last_resolved_ping = !is_timing_latency && GetTimer(latency_timer) > 0.5;

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
    if (fmsg->type == CMESSAGE_CLIPBOARD || fmsg->type == MESSAGE_TIME) {
        return SendTCPPacket(&PacketTCPContext, PACKET_MESSAGE, fmsg, GetFmsgSize(fmsg));
    } else {
        static int sent_packet_id = 0;
        sent_packet_id++;
        return SendUDPPacket(&PacketSendContext, PACKET_MESSAGE, fmsg, GetFmsgSize(fmsg),
                             sent_packet_id, -1, NULL, NULL);
    }
}

int ReceivePackets(void* opaque) {
    LOG_INFO("ReceivePackets running on Thread %p", SDL_GetThreadID(NULL));
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
            if (drop_time_ms > 0 && GetTimer(drop_test_timer) * MS_IN_SECOND > drop_time_ms) {
                is_currently_dropping = false;
                StartTimer(&drop_test_timer);
            }
        } else {
            if (drop_distance_sec > 0 && GetTimer(drop_test_timer) > drop_distance_sec) {
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
            if (lastrecv > 20.0 / MS_IN_SECOND) {
                LOG_INFO(
                    "Took more than 20ms to receive something!! Took %fms "
                    "total!",
                    lastrecv * MS_IN_SECOND);
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
                    handleServerMessage((FractalServerMessage*)packet->data,
                                        (size_t)packet->payload_size);
                    message_time += GetTimer(message_timer);
                    break;
                default:
                    LOG_WARNING("Unknown Packet");
                    break;
            }
        }
    }

    if (lastrecv > 20.0 / MS_IN_SECOND) {
        LOG_INFO("Took more than 20ms to receive something!! Took %fms total!", lastrecv * MS_IN_SECOND);
    }

    SDL_Delay(5);

    destroyUpdate();

    return 0;
}

int syncKeyboardState(void) {
    FractalClientMessage fmsg = {0};

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

    return 0;
}

int main(int argc, char* argv[]) {
    if (parseArgs(argc, argv) != 0) {
        return -1;
    }

    srand(rand() * (unsigned int)time(NULL) + rand());
    uid = rand();

    char* log_dir = getLogDir();
    if (log_dir == NULL) {
        return -1;
    }
    initLogger(log_dir);
    free(log_dir);
    if (running_ci) {
        LOG_INFO("Running in CI mode");
    }

    if (configureSSHKeys() != 0) {
        LOG_ERROR("Failed to configure SSH keys.");
        destroyLogger();
        return -1;
    }

    if (configureCache() != 0) {
        LOG_ERROR("Failed to configure cache.");
        destroyLogger();
        return -1;
    }

    if (initSocketLibrary() != 0) {
        LOG_ERROR("Failed to initialize socket library.");
        destroyLogger();
        return -1;
    }

    // Initialize the SDL window
    window = initSDL(output_width, output_height);
    if (!window) {
        LOG_ERROR("Failed to initialize SDL");
        destroySocketLibrary();
        destroyLogger();
        return -1;
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

// Windows GHA VM cannot render, it just segfaults on creating the renderer
#if defined(_WIN32)
    if (!running_ci) {
        initVideo();
    }
#else
    initVideo();
#endif

    PrintSystemInfo();
    LOG_INFO("Fractal client revision %s", FRACTAL_GIT_REVISION);

    exiting = false;
    bool failed = false;

    for (try_amount = 0; try_amount < MAX_NUM_CONNECTION_ATTEMPTS && !exiting && !failed;
         try_amount++) {
        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
            SDL_Delay(1000);
        }

        if (discoverPorts() != 0) {
            LOG_WARNING("Failed to discover ports.");
            continue;
        }

        if (connectToServer() != 0) {
            LOG_WARNING("Failed to connect to server.");
            continue;
        }

        connected = true;
        received_server_init_message = true;

        // Initialize audio and variables
        is_timing_latency = false;
        initAudio();

        // Create thread to receive all packets and handle them as needed
        run_receive_packets = true;
        SDL_Thread* receive_packets_thread =
            SDL_CreateThread(ReceivePackets, "ReceivePackets", &PacketReceiveContext);

        StartTimer(&window_resize_timer);

        if (sendTimeToServer() != 0) {
            LOG_ERROR("Failed to synchronize time with server.");
        }

        // Timer used in CI mode to exit after 1 min
        clock ci_timer;
        StartTimer(&ci_timer);

        clock ack_timer, keyboard_sync_timer;
        StartTimer(&ack_timer);
        StartTimer(&keyboard_sync_timer);

        SDL_Event sdl_msg;

        // This code will run once every millisecond
        while (connected && !exiting && !failed) {
            if (GetTimer(ack_timer) > 5) {
                Ack(&PacketSendContext);
                Ack(&PacketTCPContext);
                StartTimer(&ack_timer);
            }
            // if we are running a CI test we run for time_to_run_ci secondsA
            // before exiting
            if (running_ci && GetTimer(ci_timer) > time_to_run_ci) {
                exiting = 1;
                LOG_INFO("Exiting CI run");
            }

            if (GetTimer(keyboard_sync_timer) > 50.0 / MS_IN_SECOND) {
                if (syncKeyboardState() != 0) {
                    failed = true;
                    break;
                }
                StartTimer(&keyboard_sync_timer);
            }
            if (SDL_PollEvent(&sdl_msg)) {
                if (handleSDLEvent(&sdl_msg) != 0) {
                    failed = true;
                    break;
                }
            } else {
                SDL_Delay(1);
            }
        }

        LOG_INFO("Disconnecting...");
        if (exiting || failed || try_amount + 1 == MAX_NUM_CONNECTION_ATTEMPTS)
            sendServerQuitMessages(3);
        run_receive_packets = false;
        SDL_WaitThread(receive_packets_thread, NULL);
        destroyAudio();
        closeConnections();
    }

    LOG_INFO("Closing Client...");
    destroyVideo();
    destroySDL((SDL_Window*)window);
    destroySocketLibrary();
    destroyLogger();

    return (try_amount < 3 && !failed) ? 0 : -1;
}
