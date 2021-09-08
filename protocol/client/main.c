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
#include "sync_packets.h"
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
extern volatile int max_bitrate;
extern volatile int max_burst_bitrate;
volatile bool update_bitrate = false;

// Global state variables
volatile char binary_aes_private_key[16];
volatile char hex_aes_private_key[33];
volatile SDL_Window* window;
volatile char* window_title;
volatile bool should_update_window_title;
volatile bool run_sync_udp_packets;
volatile bool run_sync_tcp_packets;
volatile bool is_timing_latency;
volatile double latency;

volatile FractalRGBColor* native_window_color = NULL;
volatile bool native_window_color_update = false;

volatile int output_width;
volatile int output_height;
volatile char* program_name = NULL;
volatile CodecType output_codec_type = CODEC_TYPE_H264;
volatile char* server_ip;
char user_email[FRACTAL_ARGS_MAXLEN + 1];
char icon_png_filename[FRACTAL_ARGS_MAXLEN + 1];
bool using_stun = false;

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

SocketContext packet_send_udp_context = {0};
SocketContext packet_receive_udp_context = {0};
SocketContext packet_tcp_context = {0};

volatile bool connected = true;
volatile bool exiting = false;
volatile int try_amount;

// Defines
#define APP_PATH_MAXLEN 1023

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
    fmsg.keyboard_state.num_keycodes = (short)min(KEYCODE_UPPERBOUND, num_keys);
#else
    fmsg.keyboard_state.num_keycodes = fmin(KEYCODE_UPPERBOUND, num_keys);
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
    fmsg.keyboard_state.keyboard_state[FK_LGUI] = lgui_pressed;
    fmsg.keyboard_state.keyboard_state[FK_RGUI] = rgui_pressed;

    fmsg.keyboard_state.caps_lock = SDL_GetModState() & KMOD_CAPS;
    fmsg.keyboard_state.num_lock = SDL_GetModState() & KMOD_NUM;

    fmsg.keyboard_state.active_pinch = active_pinch;

    send_fmsg(&fmsg);

    return 0;
}

volatile bool continue_pumping = false;

int multithreaded_read_piped_arguments(void* keep_piping) {
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
            ack(&packet_send_udp_context);
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
        char client_app_path[APP_PATH_MAXLEN + 1];
        memset(client_app_path, 0, APP_PATH_MAXLEN + 1);

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
        if (relative_client_app_path_len < APP_PATH_MAXLEN + 1) {
#ifdef _WIN32
            int max_protocol_path_len = APP_PATH_MAXLEN + 1 - relative_client_app_path_len - 1;
            // Get the path of the current executable
            int path_read_size = GetModuleFileNameA(NULL, client_app_path, max_protocol_path_len);
            if (path_read_size > 0 && path_read_size < max_protocol_path_len) {
#elif __APPLE__
            uint32_t max_protocol_path_len =
                (uint32_t)(APP_PATH_MAXLEN + 1 - relative_client_app_path_len - 1);
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

    init_logger();
    init_statistic_logger();

    handle_single_icon_launch_client_app(argc, argv);

    init_networking();

    srand(rand() * (unsigned int)time(NULL) + rand());
    uid = rand();

    if (init_socket_library() != 0) {
        LOG_FATAL("Failed to initialize socket library.");
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
        SDL_CreateThread(multithreaded_read_piped_arguments, "PipeArgThread", &keep_piping);
    if (pipe_arg_thread == NULL) {
        exit_code = FRACTAL_EXIT_CLI;
    } else {
        SDL_Event event;
        while (continue_pumping) {
            if (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT: {
                        exiting = true;
                        keep_piping = false;
                        break;
                    }
                    case SDL_WINDOWEVENT: {
                        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                            output_width = get_window_pixel_width((SDL_Window*)window);
                            output_height = get_window_pixel_height((SDL_Window*)window);
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
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
        run_sync_udp_packets = true;
        SDL_Thread* sync_udp_packets_thread = SDL_CreateThread(
            multithreaded_sync_udp_packets, "SyncUDPPackets", &packet_receive_udp_context);

        // Create thread to send and receive TCP packets
        run_sync_tcp_packets = true;
        SDL_Thread* sync_tcp_packets_thread =
            SDL_CreateThread(multithreaded_sync_tcp_packets, "SyncTCPPackets", NULL);

        start_timer(&window_resize_timer);
        window_resize_mutex = safe_SDL_CreateMutex();

        clock keyboard_sync_timer, mouse_motion_timer, monitor_change_timer;
        start_timer(&keyboard_sync_timer);
        start_timer(&mouse_motion_timer);
        start_timer(&monitor_change_timer);

        // This code will run for as long as there are events queued, or once every millisecond if
        // there are no events queued
        while (connected && !exiting && exit_code == FRACTAL_EXIT_SUCCESS) {
            // Check if the window is minimized or occluded. If it is, we can just sleep for a bit
            // and then check again.
            // NOTE: internally within SDL, the window flags are maintained and updated upon
            // catching a window event, and `SDL_GetWindowFlags()` simply returns those stored
            // flags, so this is an efficient call
            if (SDL_GetWindowFlags((SDL_Window*)window) &
                (SDL_WINDOW_OCCLUDED | SDL_WINDOW_MINIMIZED)) {
                // Even though the window is minized/occluded, we still need to handle SDL events or
                // else the application will permanently hang
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
        run_sync_udp_packets = false;
        SDL_WaitThread(sync_udp_packets_thread, NULL);
        destroy_audio();
        close_connections();
        connected = false;
    }

    if (exit_code != FRACTAL_EXIT_SUCCESS) {
        LOG_ERROR("Failure in main loop!");
    }

    if (try_amount >= 3) {
        // we make this a LOG_WARNING so it doesn't clog up Sentry, as this
        // error happens periodically but we have recovery systems in place
        // for streaming interruption/connection loss
        LOG_WARNING("Failed to connect after three attempts!");
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
