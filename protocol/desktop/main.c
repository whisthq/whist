/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.c
 * @brief This file contains the main code that runs a Fractal client on a
 *        Windows, MacOS or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Fractal video streaming client being created and creating
its threads.
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "sentry.h"
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
#include "SDL_syswm.h"
#ifdef __APPLE__
#include "../fractal/utils/mac_utils.h"
#endif

volatile int audio_frequency = -1;

// Width and Height
volatile int server_width = -1;
volatile int server_height = -1;
volatile CodecType server_codec_type = CODEC_TYPE_UNKNOWN;

// maximum mbps
volatile int max_bitrate = STARTING_BITRATE;
volatile bool update_mbps = false;

// Global state variables
volatile char binary_aes_private_key[16];
volatile char hex_aes_private_key[33];
volatile int connection_id;
volatile SDL_Window* window;
volatile bool run_receive_packets;
volatile bool run_send_clipboard_packets;
volatile bool is_timing_latency;
volatile clock latency_timer;
volatile int ping_id;
volatile int ping_failures;

volatile int output_width;
volatile int output_height;
volatile char* program_name = NULL;
volatile CodecType output_codec_type = CODEC_TYPE_H264;
volatile char* server_ip;
int time_to_run_ci = 300;  // Seconds to run CI tests for
volatile int running_ci = 0;
char user_email[USER_EMAIL_MAXLEN];
extern char sentry_environment[FRACTAL_ENVIRONMENT_MAXLEN];
bool using_stun = true;

int UDP_port = -1;
int TCP_port = -1;
int client_id = -1;
int uid;

// Keyboard state variables
bool alt_pressed = false;
bool ctrl_pressed = false;
bool lgui_pressed = false;
bool rgui_pressed = false;

// Mouse motion state
mouse_motion_accumulation mouse_state = {0};

clock window_resize_timer;

// Function Declarations

int receive_packets(void* opaque);
int send_clipboard_packets(void* opaque);

SocketContext PacketSendContext = {0};
SocketContext PacketReceiveContext = {0};
SocketContext PacketTCPContext = {0};

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

void init_update() {
    UpdateData.tried_to_update_dimension = false;

    start_timer((clock*)&UpdateData.last_tcp_check_timer);
    start_timer((clock*)&latency_timer);
    ping_id = 1;
    ping_failures = -2;

    init_clipboard_synchronizer();
}

void destroy_update() { destroy_clipboard_synchronizer(); }

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
    if (get_timer(UpdateData.last_tcp_check_timer) > 25.0 / MS_IN_SECOND &&
        !is_clipboard_synchronizing()) {
        // Check if TCP connction is active
        int result = ack(&PacketTCPContext);
        if (result < 0) {
            LOG_ERROR("Lost TCP Connection (Error: %d)", get_last_network_error());
            // TODO: Should exit or recover protocol if TCP connection is lost
        }

        // Receive tcp buffer, if a full packet has been received
        FractalPacket* tcp_packet = read_tcp_packet(&PacketTCPContext, true);
        if (tcp_packet) {
            handle_server_message((FractalServerMessage*)tcp_packet->data,
                                (size_t)tcp_packet->payload_size);
        }

        // Update the last tcp check timer
        start_timer((clock*)&UpdateData.last_tcp_check_timer);
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
        float dpi;
        SDL_GetDisplayDPI(0, NULL, &dpi, NULL);
        fmsg.dimensions.dpi = (int)dpi;
        send_fmsg(&fmsg);
        UpdateData.tried_to_update_dimension = true;
    }

    // If the code has triggered a mbps update, then notify the server of the
    // newly desired mbps
    if (update_mbps) {
        update_mbps = false;
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_bitrate / (double)BYTES_IN_KILOBYTE / BYTES_IN_KILOBYTE;
        LOG_INFO("Asking for server MBPS to be %f", fmsg.mbps);
        send_fmsg(&fmsg);
    }

    // If it's been 1 second since the last ping, we should warn
    if (get_timer(latency_timer) > 1.0) {
        LOG_WARNING("Whoah, ping timer is way too old");
    }

    // If we're waiting for a ping, and it's been 600ms, then that ping will be
    // noted as failed
    if (is_timing_latency && get_timer(latency_timer) > 0.6) {
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
    bool taking_a_bit = is_timing_latency && get_timer(latency_timer) > 0.21 * (1 + num_ping_tries);
    // If 500ms has past since the last resolved ping, then it's been a while
    // and we should ping again (Last resolved ping is a ping that either has
    // been received, or was noted as failed)
    bool awhile_since_last_resolved_ping = !is_timing_latency && get_timer(latency_timer) > 0.5;

    // If either of the two above conditions hold, then send a new ping
    if (awhile_since_last_resolved_ping || taking_a_bit) {
        if (is_timing_latency) {
            // A continuation of an existing ping that's been taking a bit
            num_ping_tries++;
        } else {
            // A brand new ping
            ping_id++;
            is_timing_latency = true;
            start_timer((clock*)&latency_timer);
            num_ping_tries = 0;
        }

        fmsg.type = MESSAGE_PING;
        fmsg.ping_id = ping_id;

        LOG_INFO("Ping! %d", ping_id);
        send_fmsg(&fmsg);
    }
    // End Ping
}

int send_clipboard_packets(void* opaque) {
    /*
    Obtain updated clipboard and send clipboard TCP packet to server
    */

    opaque;
    LOG_INFO("SendClipboardPackets running on Thread %p", SDL_GetThreadID(NULL));

    clock clipboard_time;
    while (run_send_clipboard_packets) {
        start_timer(&clipboard_time);
        ClipboardData* clipboard = clipboard_synchronizer_get_new_clipboard();
        if (clipboard) {
            FractalClientMessage* fmsg_clipboard =
                malloc(sizeof(FractalClientMessage) + sizeof(ClipboardData) + clipboard->size);
            fmsg_clipboard->type = CMESSAGE_CLIPBOARD;
            memcpy(&fmsg_clipboard->clipboard, clipboard, sizeof(ClipboardData) + clipboard->size);
            send_fmsg(fmsg_clipboard);
            free(fmsg_clipboard);
        }

        // delay to avoid clipboard checking spam
        const int spam_time_ms = 500;
        if (get_timer(clipboard_time) < spam_time_ms / (double)MS_IN_SECOND) {
            SDL_Delay(max((int)(spam_time_ms - MS_IN_SECOND * get_timer(clipboard_time)), 1));
        }
    }

    return 0;
}
// END UPDATER CODE

int receive_packets(void* opaque) {
    LOG_INFO("receive_packets running on Thread %p", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    SocketContext socketContext = *(SocketContext*)opaque;

    /****
    Timers
    ****/

    clock world_timer;
    start_timer(&world_timer);

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
    start_timer(&last_ack);

    // NOTE: FOR DEBUGGING
    // This code will drop packets intentionally to test protocol's ability to
    // handle such events drop_distance_sec is the number of seconds in-between
    // simulated drops drop_time_ms is how long the drop will last for
    clock drop_test_timer;
    int drop_time_ms = 250;
    int drop_distance_sec = -1;
    bool is_currently_dropping = false;
    start_timer(&drop_test_timer);

    // Initialize update handler
    init_update();

    while (run_receive_packets) {
        if (get_timer(last_ack) > 5.0) {
            ack(&socketContext);
            start_timer(&last_ack);
        }

        // Handle all pending updates
        update();

        // TODO hash_time is never updated, leaving it in here in case it is
        // used in the future
        // casting to suppress warnings.
        (void)hash_time;
        // Post statistics every 5 seconds
        if (get_timer(world_timer) > 5) {
            LOG_INFO("world_time: %f", get_timer(world_timer));
            LOG_INFO("recvfrom_time: %f", recvfrom_time);
            LOG_INFO("update_video_time: %f", update_video_time);
            LOG_INFO("update_audio_time: %f", update_audio_time);
            LOG_INFO("hash_time: %f", hash_time);
            LOG_INFO("video_time: %f", video_time);
            LOG_INFO("max_video_time: %f", max_video_time);
            LOG_INFO("audio_time: %f", audio_time);
            LOG_INFO("max_audio_time: %f", max_audio_time);
            LOG_INFO("message_time: %f", message_time);
            start_timer(&world_timer);
        }

        // Video and Audio should be updated at least every 5ms
        // We will do it here, after receiving each packet or if the last recvp
        // timed out

        start_timer(&update_video_timer);
        update_video();
        update_video_time += get_timer(update_video_timer);

        start_timer(&update_audio_timer);
        update_audio();
        update_audio_time += get_timer(update_audio_timer);

        // Time the following recvfrom code
        start_timer(&recvfrom_timer);

        // START DROP EMULATION
        if (is_currently_dropping) {
            if (drop_time_ms > 0 && get_timer(drop_test_timer) * MS_IN_SECOND > drop_time_ms) {
                is_currently_dropping = false;
                start_timer(&drop_test_timer);
            }
        } else {
            if (drop_distance_sec > 0 && get_timer(drop_test_timer) > drop_distance_sec) {
                is_currently_dropping = true;
                start_timer(&drop_test_timer);
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
            packet = read_udp_packet(&socketContext);
        }

        double recvfrom_short_time = get_timer(recvfrom_timer);

        // Total amount of time spent in recvfrom / decrypt_packet
        recvfrom_time += recvfrom_short_time;
        // Total amount of cumulative time spend in recvfrom, since the last
        // time recv_size was > 0
        lastrecv += recvfrom_short_time;

        if (packet) {
            // Log if it's been a while since the last packet was received
            if (lastrecv > 50.0 / MS_IN_SECOND) {
                LOG_INFO(
                    "Took more than 50ms to receive something!! Took %fms "
                    "total!",
                    lastrecv * MS_IN_SECOND);
            }
            lastrecv = 0.0;
        }

        // LOG_INFO("Recv wait time: %f", get_timer(recvfrom_timer));

        if (packet) {
            // Check packet type and then redirect packet to the proper packet
            // handler
            switch (packet->type) {
                case PACKET_VIDEO:
                    // Video packet
                    start_timer(&video_timer);
                    receive_video(packet);
                    video_time += get_timer(video_timer);
                    max_video_time = max(max_video_time, get_timer(video_timer));
                    break;
                case PACKET_AUDIO:
                    // Audio packet
                    start_timer(&audio_timer);
                    receive_audio(packet);
                    audio_time += get_timer(audio_timer);
                    max_audio_time = max(max_audio_time, get_timer(audio_timer));
                    break;
                case PACKET_MESSAGE:
                    // A FractalServerMessage for other information
                    start_timer(&message_timer);
                    handle_server_message((FractalServerMessage*)packet->data,
                                        (size_t)packet->payload_size);
                    message_time += get_timer(message_timer);
                    break;
                default:
                    LOG_WARNING("Unknown Packet");
                    break;
            }
        }
    }

    if (lastrecv > 20.0 / MS_IN_SECOND) {
        LOG_INFO("Took more than 20ms to receive something!! Took %fms total!",
                 lastrecv * MS_IN_SECOND);
    }

    SDL_Delay(50);

    destroy_update();

    return 0;
}

int sync_keyboard_state(void) {
    // Set keyboard state initialized to null
    FractalClientMessage fmsg = {0};

    fmsg.type = MESSAGE_KEYBOARD_STATE;

    int num_keys;
    const Uint8* state = SDL_GetKeyboardState(&num_keys);
#if defined(_WIN32)
    fmsg.num_keycodes = (short)min(NUM_KEYCODES, num_keys);
#else
    fmsg.num_keycodes = fmin(NUM_KEYCODES, num_keys);
#endif

    // Copy keyboard state, but using scancodes of the keys in the current keyboard layout.
    // Must convert to/from the name of the key so SDL returns the scancode for the key in the
    // current layout rather than the scancode for the physical key.
    for (int i = 0; i < fmsg.num_keycodes; i++) {
        if (state[i]) {
            int scancode = SDL_GetScancodeFromName(SDL_GetKeyName(SDL_GetKeyFromScancode(i)));
            if (0 <= scancode && scancode < (int)sizeof(fmsg.keyboard_state)) {
                fmsg.keyboard_state[scancode] = 1;
            }
        }
    }

    // Also send caps lock and num lock status for syncronization
#ifdef __APPLE__
    fmsg.keyboard_state[FK_LCTRL] = ctrl_pressed;
    fmsg.keyboard_state[FK_LGUI] = false;
    fmsg.keyboard_state[FK_RGUI] = false;
#else
    fmsg.keyboard_state[FK_LGUI] = lgui_pressed;
    fmsg.keyboard_state[FK_RGUI] = rgui_pressed;
#endif

    fmsg.caps_lock = SDL_GetModState() & KMOD_CAPS;
    fmsg.num_lock = SDL_GetModState() & KMOD_NUM;

    send_fmsg(&fmsg);

    return 0;
}

int main(int argc, char* argv[]) {
    init_default_port_mappings();

    int ret = parse_args(argc, argv);
    if (ret == -1) {
        // invalid usage
        return -1;
    } else if (ret == 1) {
        // --help or --version
        return 0;
    }

    srand(rand() * (unsigned int)time(NULL) + rand());
    uid = rand();

    char* log_dir = get_log_dir();
    if (log_dir == NULL) {
        return -1;
    }

    // cache should be the first thing!
    if (configure_cache() != 0) {
        printf("Failed to configure cache.");
        return -1;
    }

    init_logger(log_dir);
    free(log_dir);
    // Set sentry user here based on email from command line args
    // It defaults to None, so we only inform sentry if the client app passes in a user email
    // We do this here instead of in initLogger because initLogger is used both by the client and
    // the server so we have to do it for both in their respective main.c files.
    if (strcmp(user_email, "None") != 0) {
        sentry_value_t user = sentry_value_new_object();
        sentry_value_set_by_key(user, "email", sentry_value_new_string(user_email));
    }

    if (running_ci) {
        LOG_INFO("Running in CI mode");
    }

    if (init_socket_library() != 0) {
        LOG_ERROR("Failed to initialize socket library.");
        destroy_logger();
        return -1;
    }

    // Initialize the SDL window
    window = init_sdl(output_width, output_height, (char*)program_name);
    if (!window) {
        LOG_ERROR("Failed to initialize SDL");
        destroy_socket_library();
        destroy_logger();
        return -1;
    }

    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

// Windows GHA VM cannot render, it just segfaults on creating the renderer
#if defined(_WIN32)
    if (!running_ci) {
        init_video();
    }
#else
    init_video();
#endif

    print_system_info();
    LOG_INFO("Fractal client revision %s", FRACTAL_GIT_REVISION);

    exiting = false;
    bool failed = false;

    int max_connection_attempts = MAX_INIT_CONNECTION_ATTEMPTS;
    for (try_amount = 0; try_amount < max_connection_attempts && !exiting && !failed;
         try_amount++) {
        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
            SDL_Delay(1000);
        }

        if (discover_ports(&using_stun) != 0) {
            LOG_WARNING("Failed to discover ports.");
            continue;
        }

        if (connect_to_server(using_stun) != 0) {
            LOG_WARNING("Failed to connect to server.");
            continue;
        }

        connected = true;

        // Initialize audio and variables
        // reset because now connected
        try_amount = 0;
        max_connection_attempts = MAX_RECONNECTION_ATTEMPTS;

        // Initialize audio and variables
        is_timing_latency = false;
        init_audio();

        // Create thread to receive all packets and handle them as needed
        run_receive_packets = true;
        SDL_Thread* receive_packets_thread =
            SDL_CreateThread(receive_packets, "ReceivePackets", &PacketReceiveContext);

        // Create thread to send clipboard TCP packets
        run_send_clipboard_packets = true;
        SDL_Thread* send_clipboard_thread =
            SDL_CreateThread(send_clipboard_packets, "SendClipboardPackets", NULL);

        start_timer(&window_resize_timer);

        // Timer used in CI mode to exit after 1 min
        clock ci_timer;
        start_timer(&ci_timer);

        clock ack_timer, keyboard_sync_timer, mouse_motion_timer;
        start_timer(&ack_timer);
        start_timer(&keyboard_sync_timer);
        start_timer(&mouse_motion_timer);

        SDL_Event sdl_msg;

        // This code will run for as long as there are events queued, or once every millisecond if
        // there are no events queued
        while (connected && !exiting && !failed) {
            if (get_timer(ack_timer) > 5) {
                ack(&PacketSendContext);
                ack(&PacketTCPContext);
                start_timer(&ack_timer);
            }
            // if we are running a CI test we run for time_to_run_ci seconds
            // before exiting
            if (running_ci && get_timer(ci_timer) > time_to_run_ci) {
                exiting = 1;
                LOG_INFO("Exiting CI run");
            }

            if (get_timer(keyboard_sync_timer) > 50.0 / MS_IN_SECOND) {
                if (sync_keyboard_state() != 0) {
                    failed = true;
                    break;
                }
                start_timer(&keyboard_sync_timer);
            }
            int events = SDL_PollEvent(&sdl_msg);

            if (events && handle_sdl_event(&sdl_msg) != 0) {
                // unable to handle event
                failed = true;
                break;
            }

            if (get_timer(mouse_motion_timer) > 0.5 / MS_IN_SECOND) {
                if (update_mouse_motion()) {
                    failed = true;
                    break;
                }
                start_timer(&mouse_motion_timer);
            }

            if (!events) {
                // no events found
                SDL_Delay(1);
            }
        }

        LOG_INFO("Disconnecting...");
        if (exiting || failed || try_amount + 1 == max_connection_attempts)
            send_server_quit_messages(3);
        run_receive_packets = false;
        run_send_clipboard_packets = false;
        SDL_WaitThread(receive_packets_thread, NULL);
        SDL_WaitThread(send_clipboard_thread, NULL);
        destroy_audio();
        close_connections();
    }

    if (failed) {
        sentry_value_t event = sentry_value_new_message_event(
            /*   level */ SENTRY_LEVEL_ERROR,
            /*  logger */ "client-errors",
            /* message */ "Failure in main loop");
        sentry_capture_event(event);
    }

    if (try_amount >= 3) {
        sentry_value_t event = sentry_value_new_message_event(
            /*   level */ SENTRY_LEVEL_ERROR,
            /*  logger */ "client-errors",
            /* message */ "Failed to connect after three attemps");
        sentry_capture_event(event);
    }

    LOG_INFO("Closing Client...");
    destroy_video();
    destroy_sdl((SDL_Window*)window);
    destroy_socket_library();
    destroy_logger();
    return (try_amount < 3 && !failed) ? 0 : -1;
}
