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

/*
============================
Includes
============================
*/

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <fractal/core/fractal.h>
#include <fractal/network/network.h>
#include <fractal/utils/aes.h>
#include <fractal/utils/clock.h>
#include <fractal/logging/logging.h>
#include <fractal/logging/log_statistic.h>
#include <fractal/logging/error_monitor.h>
#include "sdlscreeninfo.h"
#include "audio.h"
#include "client_utils.h"
#include "network.h"
#include "sdl_event_handler.h"
#include "sdl_utils.h"
#include "handle_server_message.h"
#include "video.h"
#include <SDL2/SDL_syswm.h>
#include <fractal/utils/color.h>
#include "native_window_utils.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <fractal/utils/mac_utils.h>
#endif  // __APPLE__

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
volatile SDL_Window* window;
volatile char* window_title;
volatile bool should_update_window_title;
volatile bool run_receive_packets;
volatile bool run_sync_tcp_packets;
volatile bool is_timing_latency;
volatile clock latency_timer;
volatile double latency;
volatile int ping_id;
volatile int ping_failures;

volatile FractalRGBColor* native_window_color = NULL;
volatile bool native_window_color_update = false;

volatile int output_width;
volatile int output_height;
volatile char* program_name = NULL;
volatile CodecType output_codec_type = CODEC_TYPE_H264;
volatile char* server_ip;
char user_email[FRACTAL_ARGS_MAXLEN + 1];
char icon_png_filename[FRACTAL_ARGS_MAXLEN + 1];
bool using_stun = true;

// given by server protocol during port discovery. tells client the ports to use
// for UDP and TCP communications.
int udp_port = -1;
int tcp_port = -1;
int client_id = -1;
int uid;

// Keyboard state variables
bool alt_pressed = false;
bool ctrl_pressed = false;
bool lgui_pressed = false;
bool rgui_pressed = false;

// Mouse motion state
MouseMotionAccumulation mouse_state = {0};

// Whether a pinch is currently active - set in sdl_event_handler.c
extern bool active_pinch;

// Window resizing state
SDL_mutex* window_resize_mutex;  // protects pending_resize_message
clock window_resize_timer;
volatile bool pending_resize_message =
    false;  // should be set to true if sdl event handler was not able to process resize event due
            // to throttling, so the main loop should process it

// Function Declarations

int receive_packets(void* opaque);

SocketContext packet_send_context = {0};
SocketContext packet_receive_context = {0};
SocketContext packet_tcp_context = {0};

volatile bool connected = true;
volatile bool exiting = false;
volatile int try_amount;

// Defines
#define MAX_APP_PATH_LEN 1024

/*
============================
Custom Types
============================
*/

// UPDATER CODE - HANDLES ALL PERIODIC UPDATES
struct UpdateData {
    bool tried_to_update_dimension;
    bool has_initialized_updated;
    clock last_tcp_check_timer;
} volatile update_data;

/*
============================
Private Function Implementations
============================
*/

static bool updater_initialized = false;

void init_update() {
    /*
        Initialize client update handler.
        Anything that will be continuously be called (within `update()`)
        that changes program state should be initialized in here.
    */

    update_data.tried_to_update_dimension = false;

    start_timer((clock*)&update_data.last_tcp_check_timer);
    start_timer((clock*)&latency_timer);

    // we initialize latency here because on macOS, latency would not initialize properly in
    // its declaration above. We start at 25ms before the first ping.
    latency = 25.0 / 1000.0;
    ping_id = 1;
    ping_failures = -2;

    init_clipboard_synchronizer(true);

    updater_initialized = true;
}

void destroy_update() {
    /*
        Runs the destruction sequence for anything that
        was initialized in `init_update()` and needs to be
        destroyed.
    */

    updater_initialized = false;
    destroy_clipboard_synchronizer();
}

void update() {
    /*
        Check all pending updates, and act on those pending updates
        to actually update the state of our program.
        This function expects to be called at minimum every 5ms to keep
        the program up-to-date.
    */

    if (!updater_initialized) {
        LOG_ERROR("Tried to update, but updater not initialized!");
    }

    FractalClientMessage fmsg = {0};

    // If we haven't yet tried to update the dimension, and the dimensions don't
    // line up, then request the proper dimension
    if (!update_data.tried_to_update_dimension &&
        (server_width != output_width || server_height != output_height ||
         server_codec_type != output_codec_type)) {
        send_message_dimensions();
        update_data.tried_to_update_dimension = true;
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

int sync_tcp_packets(void* opaque) {
    /*
        Thread to send and receive all TCP packets (clipboard and file)

        Arguments:
            opaque (void*): any arg to be passed to thread

        Return:
            (int): 0 on success
    */

    UNUSED(opaque);
    LOG_INFO("sync_tcp_packets running on Thread %p", SDL_GetThreadID(NULL));

    // TODO: compartmentalize each part into its own function
    while (run_sync_tcp_packets) {
        // RECEIVE TCP PACKET HANDLER
        // Check if TCP connection is active
        int result = ack(&packet_tcp_context);
        if (result < 0) {
            LOG_ERROR("Lost TCP Connection (Error: %d)", get_last_network_error());
            continue;
        }

        // Update the last TCP check timer
        start_timer((clock*)&update_data.last_tcp_check_timer);

        // Receive tcp buffer, if a full packet has been received
        FractalPacket* tcp_packet = read_tcp_packet(&packet_tcp_context, true);
        if (tcp_packet) {
            handle_server_message((FractalServerMessage*)tcp_packet->data,
                                  (size_t)tcp_packet->payload_size);
            free_tcp_packet(tcp_packet);
        }

        // SEND TCP PACKET HANDLERS:

        // GET CLIPBOARD HANDLER
        ClipboardData* clipboard_chunk = clipboard_synchronizer_get_next_clipboard_chunk();
        if (clipboard_chunk) {
            // Alloc an fmsg
            FractalClientMessage* fmsg_clipboard = allocate_region(
                sizeof(FractalClientMessage) + sizeof(ClipboardData) + clipboard_chunk->size);
            // Build the fmsg
            // Init metadata to 0 to prevent sending uninitialized packets over the network
            memset(fmsg_clipboard, 0, sizeof(*fmsg_clipboard));
            fmsg_clipboard->type = CMESSAGE_CLIPBOARD;
            memcpy(&fmsg_clipboard->clipboard, clipboard_chunk,
                   sizeof(ClipboardData) + clipboard_chunk->size);
            // Send the fmsg
            send_fmsg(fmsg_clipboard);
            // Free the fmsg
            deallocate_region(fmsg_clipboard);
            // Free the clipboard chunk
            deallocate_region(clipboard_chunk);
        }

        // Wait until 25 ms have elapsed since we started interacting with the TCP socket, unless
        //    the clipboard is actively being written or read
        if (!is_clipboard_synchronizing() &&
            get_timer(update_data.last_tcp_check_timer) < 25.0 / MS_IN_SECOND) {
            SDL_Delay(
                max((int)(25.0 - MS_IN_SECOND * get_timer(update_data.last_tcp_check_timer)), 1));
        }
    }

    return 0;
}

// END UPDATER CODE

// This function polls for UDP packets from the server
// NOTE: This contains a very sensitive hotpath,
// as recvp will potentially receive tens of thousands packets per second.
// The total execution time of inner for loop must not take longer than 0.01ms-0.1ms
// i.e., this function should not take any more than 10,000 assembly instructions per loop.
// Please do not put any for loops, and do not make any non-trivial system calls.
// Please label any functions in the hotpath with the following lines:
// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int receive_packets(void* opaque) {
    /*
        Receive any packets from the server and handle them appropriately

        Arguments:
            opaque (void*): any arg to be passed to thread

        Return:
            (int): 0 on success
    */

    LOG_INFO("receive_packets running on Thread %p", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    SocketContext socket_context = *(SocketContext*)opaque;

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
            ack(&socket_context);
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
            // Simulate dropping packets by just not calling recvp
            SDL_Delay(1);
            LOG_INFO("DROPPING");
            packet = NULL;
        } else {
            packet = read_udp_packet(&socket_context);
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
                LOG_WARNING(
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
    /*
        Synchronize the keyboard state of the host machine with
        that of the server by grabbing the host keyboard state and
        sending a packet to the server.

        Return:
            (int): 0 on success
    */

    // Set keyboard state initialized to null
    FractalClientMessage fmsg = {0};

    fmsg.type = MESSAGE_KEYBOARD_STATE;

    int num_keys;
    const Uint8* state = SDL_GetKeyboardState(&num_keys);
#if defined(_WIN32)
    fmsg.keyboard_state.num_keycodes = (short)min(NUM_KEYCODES, num_keys);
#else
    fmsg.keyboard_state.num_keycodes = fmin(NUM_KEYCODES, num_keys);
#endif

    // Copy keyboard state, but using scancodes of the keys in the current keyboard layout.
    // Must convert to/from the name of the key so SDL returns the scancode for the key in the
    // current layout rather than the scancode for the physical key.
    for (int i = 0; i < fmsg.keyboard_state.num_keycodes; i++) {
        if (state[i]) {
            int scancode = SDL_GetScancodeFromName(SDL_GetKeyName(SDL_GetKeyFromScancode(i)));
            if (0 <= scancode && scancode < (int)sizeof(fmsg.keyboard_state.keyboard_state)) {
                fmsg.keyboard_state.keyboard_state[scancode] = 1;
            }
        }
    }

    // Also send caps lock and num lock status for syncronization
#ifdef __APPLE__
    fmsg.keyboard_state.keyboard_state[FK_LCTRL] = ctrl_pressed;
    fmsg.keyboard_state.keyboard_state[FK_LGUI] = false;
    fmsg.keyboard_state.keyboard_state[FK_RGUI] = false;
#else
    fmsg.keyboard_state.keyboard_state[FK_LGUI] = lgui_pressed;
    fmsg.keyboard_state.keyboard_state[FK_RGUI] = rgui_pressed;
#endif

    fmsg.keyboard_state.caps_lock = SDL_GetModState() & KMOD_CAPS;
    fmsg.keyboard_state.num_lock = SDL_GetModState() & KMOD_NUM;

    fmsg.keyboard_state.active_pinch = active_pinch;

    send_fmsg(&fmsg);

    return 0;
}

volatile bool continue_pumping = false;

int read_piped_arguments_thread_function(void* keep_piping) {
    /*
        Thread function to read piped arguments from stdin

        Arguments:
            keep_piping (void*): whether to keep piping from stdin

        Returns:
            (int) 0 on success, -1 on failure
    */

    int ret = read_piped_arguments((bool*)keep_piping);
    continue_pumping = false;
    return ret;
}

static volatile bool run_renderer_thread = false;
int32_t multithreaded_renderer(void* opaque) {
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    if (init_video_renderer() != 0) {
        LOG_FATAL("Failed to initialize video renderer!");
    }

    while (run_renderer_thread) {
        // Timer used in CI mode to exit after 1 min
        static clock ci_timer;
        start_timer(&ci_timer);

        static clock ack_timer;
        start_timer(&ack_timer);

        if (get_timer(ack_timer) > 5) {
            ack(&packet_send_context);
            ack(&packet_tcp_context);
            start_timer(&ack_timer);
        }

        // Render Audio
        // is thread-safe regardless of what other function calls are being made to audio
        render_audio();
        // Render video
        render_video();

        SDL_Delay(1);
    }

    return 0;
}

void handle_single_icon_launch_client_app(int argc, char* argv[]) {
    // This function handles someone clicking the protocol icon as a means of starting Fractal by
    // instead launching the client app
    // If argc == 1 (no args passed), then check if client app path exists
    // and try to launch.
    //     This should be done first because `execl` won't cleanup any allocated resources.
    // Mac apps also sometimes pass an argument like -psn_0_2126343 to the executable.
#if defined(_WIN32) || defined(__APPLE__)
    if (argc == 1 || (argc == 2 && !strncmp(argv[1], "-psn_", 5))) {
        // hopefully the app path is not more than 1024 chars long
        char client_app_path[MAX_APP_PATH_LEN];
        memset(client_app_path, 0, MAX_APP_PATH_LEN);

#ifdef _WIN32
        const char* relative_client_app_path = "/../../Fractal.exe";
        char dir_split_char = '\\';
        size_t protocol_path_len;

#elif __APPLE__
        // This executable is located at
        //    Fractal.app/Contents/MacOS/FractalClient
        // We want to reference client app at Fractal.app/Contents/MacOS/FractalLauncher
        const char* relative_client_app_path = "/FractalLauncher";
        char dir_split_char = '/';
        int protocol_path_len;
#endif

        int relative_client_app_path_len = (int)strlen(relative_client_app_path);
        if (relative_client_app_path_len < MAX_APP_PATH_LEN) {
#ifdef _WIN32
            int max_protocol_path_len = MAX_APP_PATH_LEN - relative_client_app_path_len - 1;
            // Get the path of the current executable
            int path_read_size = GetModuleFileNameA(NULL, client_app_path, max_protocol_path_len);
            if (path_read_size > 0 && path_read_size < max_protocol_path_len) {
#elif __APPLE__
            uint32_t max_protocol_path_len =
                (uint32_t)(MAX_APP_PATH_LEN - relative_client_app_path_len - 1);
            // Get the path of the current executable
            if (_NSGetExecutablePath(client_app_path, &max_protocol_path_len) == 0) {
#endif
                // Get directory from executable path
                char* last_dir_slash_ptr = strrchr(client_app_path, dir_split_char);
                if (last_dir_slash_ptr) {
                    *last_dir_slash_ptr = '\0';
                }

                // Get the relative path to the client app from the current executable location
                protocol_path_len = strlen(client_app_path);
                if (safe_strncpy(client_app_path + protocol_path_len, relative_client_app_path,
                                 relative_client_app_path_len + 1)) {
                    LOG_INFO("Client app path: %s", client_app_path);
#ifdef _WIN32
                    // If `_execl` fails, then the program proceeds, else defers to client app
                    if (_execl(client_app_path, "Fractal.exe", NULL) < 0) {
                        LOG_INFO("_execl errno: %d errstr: %s", errno, strerror(errno));
                    }
#elif __APPLE__
                    // If `execl` fails, then the program proceeds, else defers to client app
                    if (execl(client_app_path, "Fractal", NULL) < 0) {
                        LOG_INFO("execl errno: %d errstr: %s", errno, strerror(errno));
                    }
#endif
                }
            }
        }
    }
#endif

    // END OF CHECKING IF IN PROD MODE AND TRYING TO LAUNCH CLIENT APP IF NO ARGS
}

int main(int argc, char* argv[]) {
    init_logger();
    init_statistic_logger();

    handle_single_icon_launch_client_app(argc, argv);

    init_networking();

    srand(rand() * (unsigned int)time(NULL) + rand());
    uid = rand();

    if (init_socket_library() != 0) {
        LOG_FATAL("Failed to initialize socket library.");
    }

    if (alloc_parsed_args() != 0) {
        return -1;
    }

    int ret = client_parse_args(argc, argv);
    if (ret == -1) {
        // invalid usage
        free_parsed_args();
        return -1;
    } else if (ret == 1) {
        // --help or --version
        free_parsed_args();
        return 0;
    }

    LOG_INFO("Client protocol started.");

    // Initialize the error monitor, and tell it we are the client.
    error_monitor_initialize(true);

    // Set error monitor username based on email from parsed arguments.
    error_monitor_set_username(user_email);

    // Initialize the SDL window
    window = init_sdl(output_width, output_height, (char*)program_name, icon_png_filename);

    if (!window) {
        destroy_socket_library();
        LOG_FATAL("Failed to initialize SDL");
    }

    // set the window minimum size
    SDL_SetWindowMinimumSize((SDL_Window*)window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);
    // Make sure that ctrl+click is processed as a right click on Mac
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");

    init_video();

    run_renderer_thread = true;
    SDL_Thread* renderer_thread =
        SDL_CreateThread(multithreaded_renderer, "multithreaded_renderer", NULL);

    print_system_info();
    LOG_INFO("Fractal client revision %s", fractal_git_revision());

    exiting = false;
    FractalExitCode exit_code = FRACTAL_EXIT_SUCCESS;

    // While showing the SDL loading screen, read in any piped arguments
    //    If the arguments are bad, then skip to the destruction phase
    continue_pumping = true;
    bool keep_piping = true;
    SDL_Thread* pipe_arg_thread =
        SDL_CreateThread(read_piped_arguments_thread_function, "PipeArgThread", &keep_piping);
    if (pipe_arg_thread == NULL) {
        exit_code = FRACTAL_EXIT_CLI;
    } else {
        SDL_Event event;
        while (continue_pumping) {
            if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
                exiting = true;
                keep_piping = false;
            }
        }
        int pipe_arg_ret;
        SDL_WaitThread(pipe_arg_thread, &pipe_arg_ret);
        if (pipe_arg_ret != 0) {
            exit_code = FRACTAL_EXIT_CLI;
        }
    }

    SDL_Event sdl_msg;
    // Try connection `MAX_INIT_CONNECTION_ATTEMPTS` times before
    //  closing and destroying the client.
    int max_connection_attempts = MAX_INIT_CONNECTION_ATTEMPTS;
    for (try_amount = 0;
         try_amount < max_connection_attempts && !exiting && exit_code == FRACTAL_EXIT_SUCCESS;
         try_amount++) {
        if (SDL_PollEvent(&sdl_msg) && sdl_msg.type == SDL_QUIT) {
            exiting = true;
        }

        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
            SDL_Delay(1000);
        }

        if (SDL_PollEvent(&sdl_msg) && sdl_msg.type == SDL_QUIT) {
            exiting = true;
        }

        if (discover_ports(&using_stun) != 0) {
            LOG_WARNING("Failed to discover ports.");
            continue;
        }

        if (connect_to_server(using_stun) != 0) {
            LOG_WARNING("Failed to connect to server.");
            continue;
        }

        if (SDL_PollEvent(&sdl_msg) && sdl_msg.type == SDL_QUIT) {
            exiting = true;
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
            SDL_CreateThread(receive_packets, "ReceivePackets", &packet_receive_context);

        // Create thread to send and receive TCP packets
        run_sync_tcp_packets = true;
        SDL_Thread* sync_tcp_packets_thread =
            SDL_CreateThread(sync_tcp_packets, "SendTCPPackets", NULL);

        start_timer(&window_resize_timer);
        window_resize_mutex = safe_SDL_CreateMutex();

        clock keyboard_sync_timer, mouse_motion_timer, monitor_change_timer;
        start_timer(&keyboard_sync_timer);
        start_timer(&mouse_motion_timer);
        start_timer(&monitor_change_timer);

        // This code will run for as long as there are events queued, or once every millisecond if
        // there are no events queued
        while (connected && !exiting && exit_code == FRACTAL_EXIT_SUCCESS) {
            // Check if the window is minimized. If it is, we can just sleep for a bit and then
            // check again.
            // NOTE: internally within SDL, the window flags are maintained and updated upon
            // catching a window event, and `SDL_GetWindowFlags()` simply returns those stored
            // flags, so this is an efficient call
            if (SDL_GetWindowFlags((SDL_Window*)window) & SDL_WINDOW_MINIMIZED) {
                // Even though the window is minized, we still need to handle SDL events or else the
                // application will permanently hang
                if (SDL_WaitEventTimeout(&sdl_msg, 50) && handle_sdl_event(&sdl_msg) != 0) {
                    // unable to handle event
                    exit_code = FRACTAL_EXIT_FAILURE;
                    break;
                }

                // This 50ms sleep is an arbitrary length that seems not to have noticeable latency
                // effects while still reducing CPU strain while the window is minimized
                fractal_sleep(50);
                continue;
            }
            // Check if window title should be updated
            // SDL_SetWindowTitle must be called in the main thread for
            // some clients (e.g. all Macs), hence why we update the title here
            // and not in handle_server_message
            // Since its in the PollEvent loop, it won't update if PollEvent freezes
            if (should_update_window_title) {
                if (window_title) {
                    SDL_SetWindowTitle((SDL_Window*)window, (char*)window_title);
                    free((char*)window_title);
                    window_title = NULL;
                } else {
                    LOG_ERROR("Window Title should not be null!");
                }
                should_update_window_title = false;
            }

            if (native_window_color_update && native_window_color) {
                set_native_window_color((SDL_Window*)window,
                                        *(FractalRGBColor*)native_window_color);
                native_window_color_update = false;
            }

            if (get_timer(keyboard_sync_timer) * MS_IN_SECOND > 50.0) {
                if (sync_keyboard_state() != 0) {
                    exit_code = FRACTAL_EXIT_FAILURE;
                    break;
                }
                start_timer(&keyboard_sync_timer);
            }

            // Check if window resize message should be sent to server
            if (pending_resize_message &&
                get_timer(window_resize_timer) >=
                    WINDOW_RESIZE_MESSAGE_INTERVAL / (float)MS_IN_SECOND) {
                safe_SDL_LockMutex(window_resize_mutex);
                if (pending_resize_message &&
                    get_timer(window_resize_timer) >=
                        WINDOW_RESIZE_MESSAGE_INTERVAL /
                            (float)MS_IN_SECOND) {  // double checked locking
                    pending_resize_message = false;
                    send_message_dimensions();
                    start_timer(&window_resize_timer);
                }
                safe_SDL_UnlockMutex(window_resize_mutex);
            }

            if (get_timer(monitor_change_timer) * MS_IN_SECOND > 10) {
                static int current_display = -1;
                int sdl_display = SDL_GetWindowDisplayIndex((SDL_Window*)window);

                if (current_display != sdl_display) {
                    if (current_display) {
                        // Update DPI to new monitor
                        send_message_dimensions();
                    }
                    current_display = sdl_display;
                }

                start_timer(&monitor_change_timer);
            }

            // Timeout after 50ms (On Windows, will hang when user is dragging or resizing the
            // window)
            if (SDL_WaitEventTimeout(&sdl_msg, 50) && handle_sdl_event(&sdl_msg) != 0) {
                // unable to handle event
                exit_code = FRACTAL_EXIT_FAILURE;
                break;
            }

            // After handle_sdl_event potentially captures a mouse motion,
            // We throttle it down to only update once every 0.5ms
            if (get_timer(mouse_motion_timer) * MS_IN_SECOND > 0.5) {
                if (update_mouse_motion()) {
                    exit_code = FRACTAL_EXIT_FAILURE;
                    break;
                }
                start_timer(&mouse_motion_timer);
            }
        }

        LOG_INFO("Disconnecting...");
        if (exiting || exit_code != FRACTAL_EXIT_SUCCESS ||
            try_amount + 1 == max_connection_attempts)
            send_server_quit_messages(3);

        run_sync_tcp_packets = false;
        SDL_WaitThread(sync_tcp_packets_thread, NULL);
        run_receive_packets = false;
        SDL_WaitThread(receive_packets_thread, NULL);
        destroy_audio();
        close_connections();
        connected = false;
    }

    if (exit_code != FRACTAL_EXIT_SUCCESS) {
        LOG_ERROR("Failure in main loop!");
    }

    if (try_amount >= 3) {
        LOG_ERROR("Failed to connect after three attempts!");
    }

    // Destroy any resources being used by the client
    LOG_INFO("Closing Client...");
    run_renderer_thread = false;
    SDL_WaitThread(renderer_thread, NULL);
    destroy_video();
    destroy_sdl((SDL_Window*)window);
    destroy_socket_library();
    free_parsed_args();
    destroy_logger();

    // We must call this after destroying the logger so that all
    // error monitor breadcrumbs and events can finish being reported
    // before we close the error monitor.
    error_monitor_shutdown();

    if (try_amount >= 3) {
        // We failed, so return a non-zero error code
        terminate_protocol(FRACTAL_EXIT_FAILURE);
    }

    terminate_protocol(exit_code);
}
