/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file main.c
 * @brief This file contains the main code that runs a Whist client on a
 *        Windows, MacOS or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Whist video streaming client being created and creating
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
#include <whist/core/whist.h>
#include <whist/network/network.h>
#include <whist/utils/aes.h>
#include <whist/utils/clock.h>
#include <whist/logging/logging.h>
#include <whist/logging/log_statistic.h>
#include <whist/logging/error_monitor.h>
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
#include <whist/utils/color.h>
#include "native_window_utils.h"
#include "client_statistic.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <whist/utils/mac_utils.h>
#endif  // __APPLE__

// N.B.: Please don't put globals here, since main.c won't be included when the testing suite is
// used instead

// Width and Height
extern volatile int server_width;
extern volatile int server_height;
extern volatile CodecType server_codec_type;

// maximum mbps
extern volatile int max_bitrate;
extern volatile int max_burst_bitrate;
extern volatile bool update_bitrate;

// Global state variables
extern volatile char binary_aes_private_key[16];
extern volatile char hex_aes_private_key[33];
extern volatile SDL_Window* window;
volatile bool is_timing_latency;
extern volatile double latency;

extern volatile int output_width;
extern volatile int output_height;
extern volatile char* program_name;
extern volatile CodecType output_codec_type;
extern volatile char* server_ip;
extern char user_email[WHIST_ARGS_MAXLEN + 1];
extern char icon_png_filename[WHIST_ARGS_MAXLEN + 1];
extern bool using_stun;

// given by server protocol during port discovery. tells client the ports to use
// for UDP and TCP communications.
extern int udp_port;
extern int tcp_port;
extern int uid;

// Keyboard state variables
extern bool alt_pressed;
extern bool ctrl_pressed;
extern bool lgui_pressed;
extern bool rgui_pressed;

// Mouse motion state
extern MouseMotionAccumulation mouse_state;

// Whether a pinch is currently active - set in sdl_event_handler.c
extern bool active_pinch;

// Window resizing state
extern WhistMutex window_resize_mutex;  // protects pending_resize_message
extern clock window_resize_timer;
extern volatile bool pending_resize_message;

extern volatile bool connected;
extern volatile bool client_exiting;
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
    WhistClientMessage wcmsg = {0};

    wcmsg.type = MESSAGE_KEYBOARD_STATE;

    int num_keys;
    const Uint8* state = SDL_GetKeyboardState(&num_keys);
#if defined(_WIN32)
    wcmsg.keyboard_state.num_keycodes = (short)min(KEYCODE_UPPERBOUND, num_keys);
#else
    wcmsg.keyboard_state.num_keycodes = fmin(KEYCODE_UPPERBOUND, num_keys);
#endif

    // Copy keyboard state, but using scancodes of the keys in the current keyboard layout.
    // Must convert to/from the name of the key so SDL returns the scancode for the key in the
    // current layout rather than the scancode for the physical key.
    for (int i = 0; i < wcmsg.keyboard_state.num_keycodes; i++) {
        if (state[i]) {
            int scancode = SDL_GetScancodeFromName(SDL_GetKeyName(SDL_GetKeyFromScancode(i)));
            if (0 <= scancode && scancode < (int)sizeof(wcmsg.keyboard_state.state)) {
                wcmsg.keyboard_state.state[scancode] = 1;
            }
        }
    }

    // Also send caps lock and num lock status for syncronization
    wcmsg.keyboard_state.state[FK_LGUI] = lgui_pressed;
    wcmsg.keyboard_state.state[FK_RGUI] = rgui_pressed;

    wcmsg.keyboard_state.caps_lock = SDL_GetModState() & KMOD_CAPS;
    wcmsg.keyboard_state.num_lock = SDL_GetModState() & KMOD_NUM;

    wcmsg.keyboard_state.active_pinch = active_pinch;

    send_wcmsg(&wcmsg);

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
        render_audio();
        render_video();
        SDL_Delay(1);
    }

    return 0;
}

void handle_single_icon_launch_client_app(int argc, char* argv[]) {
    // This function handles someone clicking the protocol icon as a means of starting Whist by
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
        const char* relative_client_app_path = "/../../Whist.exe";
        char dir_split_char = '\\';
        size_t protocol_path_len;

#elif __APPLE__
        // This executable is located at
        //    Whist.app/Contents/MacOS/WhistClient
        // We want to reference client app at Whist.app/Contents/MacOS/WhistLauncher
        const char* relative_client_app_path = "/WhistLauncher";
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
                    if (_execl(client_app_path, "Whist.exe", NULL) < 0) {
                        LOG_INFO("_execl errno: %d errstr: %s", errno, strerror(errno));
                    }
#elif __APPLE__
                    // If `execl` fails, then the program proceeds, else defers to client app
                    if (execl(client_app_path, "Whist", NULL) < 0) {
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

    // init_networking needs to be called before `client_parse_args` because it modifies
    //     `port_mappings`
    whist_init_networking();

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

    whist_init_logger();
    init_client_statistics();
    whist_init_statistic_logger(CLIENT_NUM_METRICS, client_statistic_info,
                                STATISTICS_FREQUENCY_IN_SEC);

    handle_single_icon_launch_client_app(argc, argv);

    srand(rand() * (unsigned int)time(NULL) + rand());
    uid = rand();

    LOG_INFO("Client protocol started.");

    // Initialize the error monitor, and tell it we are the client.
    whist_error_monitor_initialize(true);

    // Set error monitor username based on email from parsed arguments.
    error_monitor_set_username(user_email);

    WhistThread renderer_thread = NULL;

    print_system_info();
    LOG_INFO("Whist client revision %s", whist_git_revision());

    client_exiting = false;
    WhistExitCode exit_code = WHIST_EXIT_SUCCESS;

    // While showing the SDL loading screen, read in any piped arguments
    //    If the arguments are bad, then skip to the destruction phase
    continue_pumping = true;
    bool keep_piping = true;
    WhistThread pipe_arg_thread =
        whist_create_thread(multithreaded_read_piped_arguments, "PipeArgThread", &keep_piping);
    size_t while_loop_iter = 0;
    size_t while_loop_event = 0;
    if (pipe_arg_thread == NULL) {
        exit_code = WHIST_EXIT_CLI;
    } else {
        SDL_Event event;
        while (continue_pumping) {
            while_loop_iter++;
            // If we don't delay, your computer's CPU will freak out
            SDL_Delay(50);
            if (SDL_PollEvent(&event)) {
                while_loop_event++;
                switch (event.type) {
                    case SDL_QUIT: {
                        client_exiting = true;
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
        whist_wait_thread(pipe_arg_thread, &pipe_arg_ret);
        if (pipe_arg_ret != 0) {
            exit_code = WHIST_EXIT_CLI;
        }
    }
    LOG_INFO("Entered while loop %lu times, with %lu events (%lu)%", while_loop_iter, while_loop_event, (while_loop_event/while_loop_iter) * 100);

    SDL_Event sdl_msg;
    // Try connection `MAX_INIT_CONNECTION_ATTEMPTS` times before
    //  closing and destroying the client.
    int max_connection_attempts = MAX_INIT_CONNECTION_ATTEMPTS;
    for (try_amount = 0;
         try_amount < max_connection_attempts && !client_exiting && exit_code == WHIST_EXIT_SUCCESS;
         try_amount++) {
        if (SDL_PollEvent(&sdl_msg) && sdl_msg.type == SDL_QUIT) {
            client_exiting = true;
        }

        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
            SDL_Delay(1000);
        }

        if (SDL_PollEvent(&sdl_msg) && sdl_msg.type == SDL_QUIT) {
            client_exiting = true;
        }

        clock handshake_time;
        start_timer(&handshake_time);  // start timer for measuring handshake time
        LOG_INFO("Begin measuring handshake, current time = %s", current_time_str());

        if (discover_ports(&using_stun) != 0) {
            LOG_WARNING("Failed to discover ports.");
            continue;
        }

        LOG_INFO("Time elasped after discover_ports() = %f, current time = %s",
                 get_timer(handshake_time), current_time_str());

        if (connect_to_server(using_stun) != 0) {
            LOG_WARNING("Failed to connect to server.");
            continue;
        }

        LOG_INFO("Time elasped after connect_to_server() = %f, current time= %s",
                 get_timer(handshake_time), current_time_str());

        if (SDL_PollEvent(&sdl_msg) && sdl_msg.type == SDL_QUIT) {
            client_exiting = true;
        }

        connected = true;

        // Initialize the SDL window (and only do this once!)
        if (!window) {
            window = init_sdl(output_width, output_height, (char*)program_name, icon_png_filename);
            if (!window) {
                LOG_FATAL("Failed to initialize SDL");
            }
        }

        // set the window minimum size
        SDL_SetWindowMinimumSize((SDL_Window*)window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);
        // Make sure that ctrl+click is processed as a right click on Mac
        SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
        SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

        init_video();

        run_renderer_thread = true;
        renderer_thread =
            whist_create_thread(multithreaded_renderer, "multithreaded_renderer", NULL);

        // Initialize audio and variables
        // reset because now connected
        try_amount = 0;
        max_connection_attempts = MAX_RECONNECTION_ATTEMPTS;

        // Initialize audio and variables
        is_timing_latency = false;
        init_audio();

        // Initialize the clipboard and file synchronizers. This must happen before we start
        // the udp/tcp threads
        init_clipboard_synchronizer(true);
        init_file_synchronizer();

        // Create threads to receive udp/tcp packets and handle them as needed
        init_packet_synchronizers();

        start_timer(&window_resize_timer);
        window_resize_mutex = whist_create_mutex();

        clock keyboard_sync_timer, mouse_motion_timer, monitor_change_timer;
        start_timer(&keyboard_sync_timer);
        start_timer(&mouse_motion_timer);
        start_timer(&monitor_change_timer);

        // This code will run for as long as there are events queued, or once every millisecond if
        // there are no events queued
        while (connected && !client_exiting && exit_code == WHIST_EXIT_SUCCESS) {
            // Check if the window is minimized or occluded. If it is, we can just sleep for a bit
            // and then check again.
            if (!sdl_is_window_visible()) {
                // Even though the window is minized/occluded, we still need to handle SDL events or
                // else the application will permanently hang
                if (SDL_WaitEventTimeout(&sdl_msg, 50) && handle_sdl_event(&sdl_msg) != 0) {
                    // unable to handle event
                    exit_code = WHIST_EXIT_FAILURE;
                    break;
                }

                // This 50ms sleep is an arbitrary length that seems not to have noticeable latency
                // effects while still reducing CPU strain while the window is minimized
                whist_sleep(50);
                continue;
            }

            update_pending_sdl_tasks();

            if (get_timer(keyboard_sync_timer) * MS_IN_SECOND > 50.0) {
                if (sync_keyboard_state() != 0) {
                    exit_code = WHIST_EXIT_FAILURE;
                    break;
                }
                start_timer(&keyboard_sync_timer);
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
                exit_code = WHIST_EXIT_FAILURE;
                break;
            }

            // After handle_sdl_event potentially captures a mouse motion,
            // We throttle it down to only update once every 0.5ms
            if (get_timer(mouse_motion_timer) * MS_IN_SECOND > 0.5) {
                if (update_mouse_motion()) {
                    exit_code = WHIST_EXIT_FAILURE;
                    break;
                }
                start_timer(&mouse_motion_timer);
            }
        }

        LOG_INFO("Disconnecting...");
        if (client_exiting || exit_code != WHIST_EXIT_SUCCESS ||
            try_amount + 1 == max_connection_attempts)
            send_server_quit_messages(3);

        destroy_packet_synchronizers();
        destroy_clipboard_synchronizer();
        destroy_audio();
        close_connections();
        run_renderer_thread = false;
        whist_wait_thread(renderer_thread, NULL);
        destroy_video();
        connected = false;
    }

    if (exit_code != WHIST_EXIT_SUCCESS) {
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
    destroy_sdl((SDL_Window*)window);
    free_parsed_args();
    destroy_statistic_logger();
    destroy_logger();

    // We must call this after destroying the logger so that all
    // error monitor breadcrumbs and events can finish being reported
    // before we close the error monitor.
    error_monitor_shutdown();

    LOG_INFO("Protocol has shutdown gracefully");

    if (try_amount >= 3) {
        // We failed to connect, so return a failure error code
        return WHIST_EXIT_FAILURE;
    } else {
        return exit_code;
    }
}
