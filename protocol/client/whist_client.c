/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file main.c
 * @brief This file contains the main code that runs a Whist client on a
 *        Windows, MacOS or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Whist video streaming client being created and creating
its threads.
*/

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
#include <whist/utils/os_utils.h>
#include <whist/logging/logging.h>
#include <whist/logging/log_statistic.h>
#include <whist/logging/error_monitor.h>
#include "sdlscreeninfo.h"
#include "whist_client.h"
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
#include "renderer.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <whist/utils/mac_utils.h>
#endif  // __APPLE__

// N.B.: Please don't put globals here, since main.c won't be included when the testing suite is
// used instead

// Global state variables
extern volatile char binary_aes_private_key[16];
extern volatile char hex_aes_private_key[33];
extern volatile SDL_Window* window;
volatile bool is_timing_latency;
extern volatile double latency;

extern volatile int output_width;
extern volatile int output_height;
extern volatile char* program_name;
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
extern WhistTimer window_resize_timer;
extern volatile bool pending_resize_message;

// The state of the client, i.e. whether it's connected to a server or not
extern volatile bool connected;

extern volatile bool client_exiting;
volatile int try_amount;

extern volatile char* new_tab_url;

// Defines
#define APP_PATH_MAXLEN 1023

static int sync_keyboard_state(void) {
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
            if (0 <= i && i < (int)sizeof(wcmsg.keyboard_state.state)) {
                wcmsg.keyboard_state.state[i] = 1;
            }
        }
    }

    // Also send caps lock and num lock status for syncronization
    wcmsg.keyboard_state.state[FK_LGUI] = lgui_pressed;
    wcmsg.keyboard_state.state[FK_RGUI] = rgui_pressed;

    wcmsg.keyboard_state.caps_lock = SDL_GetModState() & KMOD_CAPS;
#ifndef __APPLE__
    // On macs, the num lock has no functionality, so we should always have it disabled
    wcmsg.keyboard_state.num_lock = SDL_GetModState() & KMOD_NUM;
#endif

    wcmsg.keyboard_state.active_pinch = active_pinch;

    // Grabs the keyboard layout as well
    wcmsg.keyboard_state.layout = get_keyboard_layout();

    send_wcmsg(&wcmsg);

    return 0;
}

static volatile bool continue_pumping = false;

static int multithreaded_read_piped_arguments(void* keep_piping) {
    /*
        Thread function to read piped arguments from stdin

        Arguments:
            keep_piping (void*): whether to keep piping from stdin

        Returns:
            (int) 0 on success, -1 on failure
    */

    int ret = read_piped_arguments((bool*)keep_piping, /*run_only_once=*/false);
    continue_pumping = false;
    return ret;
}

static void handle_single_icon_launch_client_app(int argc, char* argv[]) {
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

int whist_client_main(int argc, char* argv[]) {
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

    whist_init_subsystems();

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
    whist_error_monitor_set_username(user_email);

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
    if (pipe_arg_thread == NULL) {
        exit_code = WHIST_EXIT_CLI;
    } else {
        SDL_Event event;
        while (continue_pumping) {
            // If we don't delay, your computer's CPU will freak out
            SDL_Delay(50);
            if (SDL_PollEvent(&event)) {
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

        WhistTimer handshake_time;
        start_timer(&handshake_time);  // start timer for measuring handshake time
        LOG_INFO("Begin measuring handshake, current time = %s", current_time_str());

        if (discover_ports(&using_stun) != 0) {
            LOG_WARNING("Failed to discover ports.");
            continue;
        }

        // Log to METRIC for cross-session tracking and INFO for developer-facing logging
        double discover_ports_time = get_timer(&handshake_time);
        LOG_INFO("Time elasped after discover_ports() = %f, current time = %s", discover_ports_time,
                 current_time_str());
        LOG_METRIC("\"Handshake_discover_ports_time\" : %f", discover_ports_time);

        if (connect_to_server(using_stun) != 0) {
            LOG_WARNING("Failed to connect to server.");
            continue;
        }

        // Log to METRIC for cross-session tracking and INFO for developer-facing logging
        double connect_to_server_time = get_timer(&handshake_time);
        LOG_INFO("Time elasped after connect_to_server() = %f, current time= %s",
                 connect_to_server_time, current_time_str());
        LOG_METRIC("\"Handshake_connect_to_server_time\" : %f", connect_to_server_time);

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

        // Send our initial width/height/codec to the server,
        // so it can synchronize with us
        send_message_dimensions();

        is_timing_latency = false;
        // Initialize audio and video renderer system
        WhistRenderer* whist_renderer = init_renderer(output_width, output_height);

        // reset because now connected
        try_amount = 0;
        max_connection_attempts = MAX_RECONNECTION_ATTEMPTS;

        // Initialize the clipboard and file synchronizers. This must happen before we start
        // the udp/tcp threads
        init_clipboard_synchronizer(true);
        init_file_synchronizer(FILE_TRANSFER_CLIENT_DOWNLOAD);

        // Create threads to receive udp/tcp packets and handle them as needed
        // Pass the whist_renderer so that udp packets can be fed into it
        init_packet_synchronizers(whist_renderer);

        start_timer(&window_resize_timer);
        window_resize_mutex = whist_create_mutex();

        WhistTimer keyboard_sync_timer, monitor_change_timer;
        start_timer(&keyboard_sync_timer);
        start_timer(&monitor_change_timer);

        // Timer ensures we check piped args for potential URLs to open no more than once every
        // 50ms. This prevents CPU overload.
        WhistTimer new_tab_url_timer;
        start_timer(&new_tab_url_timer);

        WhistTimer window_fade_timer;
        start_timer(&window_fade_timer);

        // This code will run for as long as there are events queued, or once every millisecond if
        // there are no events queued
        while (connected && !client_exiting && exit_code == WHIST_EXIT_SUCCESS) {
            // This should be called BEFORE the call to read_piped_arguments,
            // otherwise one URL may get lost.
            send_new_tab_url_if_needed();

            // Update any pending SDL tasks
            sdl_update_pending_tasks();

            // Try rendering anything out, if there's something to render out
            renderer_try_render(whist_renderer);

            // We _must_ keep make calling this function as much as we can,
            // or else the user will get beachball / "Whist Not Responding"
            // Note, that the OS will sometimes hang this function for an arbitrarily long time
            if (!sdl_handle_events()) {
                // unable to handle event
                exit_code = WHIST_EXIT_FAILURE;
                break;
            }

            if (get_timer(&new_tab_url_timer) * MS_IN_SECOND > 50.0) {
                bool keep_piping2 = true;
                int piped_args_ret = read_piped_arguments(&keep_piping2, /*run_only_one=*/true);
                if (piped_args_ret == -1) {
                    LOG_ERROR("Failed to read piped arguments - exiting client");
                    exit_code = WHIST_EXIT_FAILURE;
                } else if (piped_args_ret == 1) {
                    exit_code = WHIST_EXIT_CLI;
                }
                start_timer(&new_tab_url_timer);
            }

            if (get_timer(&keyboard_sync_timer) * MS_IN_SECOND > 50.0) {
                if (sync_keyboard_state() != 0) {
                    exit_code = WHIST_EXIT_FAILURE;
                    break;
                }
                start_timer(&keyboard_sync_timer);
            }

            if (get_timer(&monitor_change_timer) * MS_IN_SECOND > 10) {
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

            // Check if the window is minimized or occluded.
            if (!sdl_is_window_visible()) {
                // If it is, we can sleep for a good while to keep CPU usage very low.
                whist_sleep(10);
            } else {
                // Otherwise, we sleep for a much shorter time to stay responsive,
                // but we still don't let the loop be tight (in order to improve battery life)
                whist_usleep(0.25 * US_IN_MS);
            }
        }

        LOG_INFO("Disconnecting...");
        if (client_exiting || exit_code != WHIST_EXIT_SUCCESS ||
            try_amount + 1 == max_connection_attempts) {
            send_server_quit_messages(3);
        }

        // Destroy the network system
        destroy_packet_synchronizers();
        destroy_clipboard_synchronizer();
        destroy_file_synchronizer();
        close_connections();

        // Destroy the renderer
        destroy_renderer(whist_renderer);

        // Mark as disconnected
        connected = false;
    }

    if (exit_code != WHIST_EXIT_SUCCESS) {
        LOG_ERROR("Failure in main loop! Exiting with code %d", exit_code);
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
    whist_error_monitor_shutdown();

    LOG_INFO("Protocol has shutdown gracefully");

    if (try_amount >= 3) {
        // We failed to connect, so return a failure error code
        return WHIST_EXIT_FAILURE;
    } else {
        return exit_code;
    }
}
