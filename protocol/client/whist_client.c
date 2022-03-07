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
#include <whist/utils/sysinfo.h>
#include <whist/logging/logging.h>
#include <whist/logging/log_statistic.h>
#include <whist/logging/error_monitor.h>
#include <whist/file/file_upload.h>
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
#include "renderer.h"
#include <whist/debug/debug_console.h>
#include "whist/utils/command_line.h"
#include "audio_path.h"

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

extern volatile int output_width;
extern volatile int output_height;
static char* program_name;
static char* server_ip;
static char* user_email;
static char* icon_png_filename;
extern bool using_stun;

// Keyboard state variables
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
static int try_amount;

static char* new_tab_urls;

// Used to check if we need to call filepicker from main thread
extern bool upload_initiated;

// Defines
#define APP_PATH_MAXLEN 1023
#ifndef LOG_CPU_USAGE
#define LOG_CPU_USAGE 0
#endif

// Command-line options.

COMMAND_LINE_STRING_OPTION(user_email, 'u', "user", WHIST_ARGS_MAXLEN,
                           "Tell Whist the user's email.  Default: None.")
COMMAND_LINE_STRING_OPTION(icon_png_filename, 'i', "icon", WHIST_ARGS_MAXLEN,
                           "Set the protocol window icon from a 64x64 pixel png file.")
COMMAND_LINE_STRING_OPTION(new_tab_urls, 'x', "new-tab-url", MAX_URL_LENGTH* MAX_NEW_TAB_URLS,
                           "URL to open in new tab.")
COMMAND_LINE_STRING_OPTION(program_name, 'n', "name", SIZE_MAX,
                           "Set the window title.  Default: Whist.")
COMMAND_LINE_STRING_OPTION(server_ip, 0, "server-ip", IP_MAXLEN, "Set the server IP to connect to.")

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

    // Also send caps lock and num lock status for synchronization
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

static void handle_single_icon_launch_client_app(int argc, const char* argv[]) {
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

static void initiate_file_upload(void) {
    /*
        Pull up system file dialog and set selection as transfering file
    */

    const char* ns_picked_file_path = whist_file_upload_get_picked_file();
    if (ns_picked_file_path) {
        file_synchronizer_set_file_reading_basic_metadata(ns_picked_file_path,
                                                          FILE_TRANSFER_SERVER_UPLOAD, NULL);
        LOG_INFO("Upload has been initiated");
    } else {
        LOG_INFO("No file selected");
        WhistClientMessage wcmsg = {0};
        wcmsg.type = MESSAGE_FILE_UPLOAD_CANCEL;
        send_wcmsg(&wcmsg);
    }
    upload_initiated = false;
}

static void send_new_tab_urls_if_needed(WhistFrontend* frontend) {
    // Send any new URL to the server
    if (new_tab_urls) {
        LOG_INFO("Sending message to open URL in new tab %s", new_tab_urls);
        const size_t urls_length = strlen((const char*)new_tab_urls);
        const size_t wcmsg_size = sizeof(WhistClientMessage) + urls_length + 1;
        WhistClientMessage* wcmsg = safe_malloc(wcmsg_size);
        memset(wcmsg, 0, sizeof(*wcmsg));
        wcmsg->type = MESSAGE_OPEN_URL;
        memcpy(&wcmsg->urls_to_open, (const char*)new_tab_urls, urls_length + 1);
        send_wcmsg(wcmsg);
        free(wcmsg);

        free((void*)new_tab_urls);
        new_tab_urls = NULL;

        // Unmimimize the window if needed
        if (!whist_frontend_is_window_visible(frontend)) {
            SDL_RestoreWindow((SDL_Window*)window);
        }
    }
}

int whist_client_main(int argc, const char* argv[]) {
    int ret = client_parse_args(argc, argv);
    if (ret == -1) {
        // invalid usage
        return WHIST_EXIT_CLI;
    } else if (ret == 1) {
        // --help or --version
        return WHIST_EXIT_SUCCESS;
    }

    whist_init_subsystems();

    // the logic inside guarantees debug console is only enabled for debug build
    init_debug_console();

    whist_init_statistic_logger(STATISTICS_FREQUENCY_IN_SEC);

    WhistFrontend* frontend = whist_frontend_create_sdl();

    if (!frontend) {
        LOG_FATAL("Failed whist_frontend_create_sdl()!");
    }

#if USE_NEW_AUDIO_PATH
    audio_path_init(get_decoded_bytes_per_frame());
    WhistThread dedicated_audio_render_thread = init_dedicated_audio_render_thread(frontend);
#endif

    handle_single_icon_launch_client_app(argc, argv);

    srand(rand() * (unsigned int)time(NULL) + rand());

    LOG_INFO("Client protocol started.");

    // Initialize the error monitor, and tell it we are the client.
    whist_error_monitor_initialize(true);

    // Set error monitor username based on email from parsed arguments.
    whist_error_monitor_set_username(user_email);

    print_system_info();
    LOG_INFO("Whist client revision %s", whist_git_revision());

    client_exiting = false;
    WhistExitCode exit_code = WHIST_EXIT_SUCCESS;

    // Read in any piped arguments. If the arguments are bad, then skip to the destruction phase
    switch (read_piped_arguments(false)) {
        case -2: {
            // Fatal reading pipe or similar
            LOG_ERROR("Failed to read piped arguments -- exiting");
            exit_code = WHIST_EXIT_FAILURE;
            break;
        }
        case -1: {
            // Invalid arguments
            LOG_ERROR("Invalid piped arguments -- exiting");
            exit_code = WHIST_EXIT_CLI;
            break;
        }
        case 1: {
            // Arguments prompt graceful exit
            LOG_INFO("Piped argument prompts graceful exit");
            exit_code = WHIST_EXIT_SUCCESS;
            client_exiting = true;
            break;
        }
        default: {
            // Success, so nothing to do
            break;
        }
    }

    // Try connection `MAX_INIT_CONNECTION_ATTEMPTS` times before
    //  closing and destroying the client.
    for (try_amount = 0; try_amount < MAX_INIT_CONNECTION_ATTEMPTS && !client_exiting &&
                         exit_code == WHIST_EXIT_SUCCESS;
         try_amount++) {
        if (try_amount > 0) {
            LOG_WARNING("Trying to recover the server connection...");
            // TODO: This is a sleep 1000, but I don't think we should ever show the user
            // a frozen window for 1 second if we're not connected to the server. Better to
            // show a "reconnecting" message within the main loop.
            whist_sleep(1000);
        }

        // Initialize the SDL window (and only do this once!)
        if (!window) {
            window = init_sdl(output_width, output_height, (char*)program_name, icon_png_filename,
                              frontend);
            if (!window) {
                LOG_FATAL("Failed to initialize SDL");
            }
        }

        // The lines below may be called multiple times,
        // Please ensure they get destroyed properly

        // Initialize audio and video renderer system
        WhistRenderer* whist_renderer = init_renderer(frontend, output_width, output_height);

        // Initialize the clipboard and file synchronizers. This must happen before we start
        // the udp/tcp threads
        init_clipboard_synchronizer(true);
        init_file_synchronizer(FILE_TRANSFER_CLIENT_DOWNLOAD);

        // Add listeners for global file drag events
        initialize_out_of_window_drag_handlers(frontend);

        start_timer(&window_resize_timer);
        window_resize_mutex = whist_create_mutex();

        WhistTimer keyboard_sync_timer, monitor_change_timer;
        start_timer(&keyboard_sync_timer);
        start_timer(&monitor_change_timer);

        // Timer ensures we check piped args for potential URLs to open no more than once every
        // 50ms. This prevents CPU overload.
        WhistTimer new_tab_urls_timer;
        start_timer(&new_tab_urls_timer);

        WhistTimer window_fade_timer;
        start_timer(&window_fade_timer);

        WhistTimer cpu_usage_statistics_timer;
        start_timer(&cpu_usage_statistics_timer);

        WhistTimer handshake_time;
        start_timer(&handshake_time);  // start timer for measuring handshake time
        LOG_INFO("Begin measuring handshake");

        if (connect_to_server(server_ip, using_stun, user_email) != 0) {
            // This must destroy everything initialized above this line
            LOG_WARNING("Failed to connect to server.");
            destroy_out_of_window_drag_handlers();
            destroy_file_synchronizer();
            destroy_clipboard_synchronizer();
            destroy_renderer(whist_renderer);
            continue;
        }
        // Reset try counter, because connection succeeded
        try_amount = 0;
        connected = true;

        // Log to METRIC for cross-session tracking and INFO for developer-facing logging
        double connect_to_server_time = get_timer(&handshake_time);
        LOG_INFO("Time elasped after connect_to_server() = %f", connect_to_server_time);
        LOG_METRIC("\"HANDSHAKE_CONNECT_TO_SERVER_TIME\" : %f", connect_to_server_time);

        // Create threads to receive udp/tcp packets and handle them as needed
        // Pass the whist_renderer so that udp packets can be fed into it
        init_packet_synchronizers(whist_renderer);

        // Send our initial width/height/codec to the server,
        // so it can synchronize with us
        send_message_dimensions(frontend);

        // This code will run for as long as there are events queued, or once every millisecond if
        // there are no events queued
        while (connected && !client_exiting && exit_code == WHIST_EXIT_SUCCESS) {
            // This should be called BEFORE the call to read_piped_arguments,
            // otherwise one URL may get lost.
            send_new_tab_urls_if_needed(frontend);

            // Update any pending SDL tasks
            sdl_update_pending_tasks(frontend);

            // Try rendering anything out, if there's something to render out
            renderer_try_render(whist_renderer);

            // Log cpu usage once per second. Only enable this when LOG_CPU_USAGE flag is set
            // because getting cpu usage statistics is expensive.
            if (LOG_CPU_USAGE && get_timer(&cpu_usage_statistics_timer) > 1) {
                double cpu_usage = get_cpu_usage();
                log_double_statistic(CLIENT_CPU_USAGE, cpu_usage);
                start_timer(&cpu_usage_statistics_timer);
            }

            // We _must_ keep make calling this function as much as we can,
            // or else the user will get beachball / "Whist Not Responding"
            // Note, that the OS will sometimes hang this function for an arbitrarily long time
            if (!sdl_handle_events(frontend)) {
                // unable to handle event
                exit_code = WHIST_EXIT_FAILURE;
                break;
            }

            if (get_timer(&new_tab_urls_timer) * MS_IN_SECOND > 50.0) {
                int piped_args_ret = read_piped_arguments(true);
                switch (piped_args_ret) {
                    case -2: {
                        // Fatal reading pipe or similar
                        LOG_ERROR("Failed to read piped arguments -- exiting");
                        exit_code = WHIST_EXIT_FAILURE;
                        break;
                    }
                    case -1: {
                        // Invalid arguments
                        LOG_ERROR("Invalid piped arguments -- exiting");
                        exit_code = WHIST_EXIT_CLI;
                        break;
                    }
                    case 1: {
                        // Arguments prompt graceful exit
                        LOG_INFO("Piped argument prompts graceful exit");
                        exit_code = WHIST_EXIT_SUCCESS;
                        client_exiting = true;
                        break;
                    }
                    default: {
                        // Success, so nothing to do
                        break;
                    }
                }
                start_timer(&new_tab_urls_timer);
            }

            if (get_timer(&keyboard_sync_timer) * MS_IN_SECOND > 50.0) {
                if (sync_keyboard_state() != 0) {
                    exit_code = WHIST_EXIT_FAILURE;
                    break;
                }
                start_timer(&keyboard_sync_timer);
            }

            if (get_timer(&monitor_change_timer) * MS_IN_SECOND > 10) {
                static int cached_display_index = -1;
                int current_display_index;
                if (whist_frontend_get_window_display_index(frontend, &current_display_index) ==
                    WHIST_SUCCESS) {
                    if (cached_display_index != current_display_index) {
                        if (cached_display_index) {
                            // Update DPI to new monitor
                            send_message_dimensions(frontend);
                        }
                        cached_display_index = current_display_index;
                    }
                } else {
                    LOG_ERROR("Failed to get display index");
                }

                start_timer(&monitor_change_timer);
            }

            // Check if the window is minimized or occluded.
            if (!whist_frontend_is_window_visible(frontend)) {
                // If it is, we can sleep for a good while to keep CPU usage very low.
                whist_sleep(10);
            } else {
                // Otherwise, we sleep for a much shorter time to stay responsive,
                // but we still don't let the loop be tight (in order to improve battery life)
                whist_usleep(0.25 * US_IN_MS);
            }

            // Check if file upload has been initiated and initiated selection dialog if so
            if (upload_initiated) {
                initiate_file_upload();
            }
        }

        LOG_INFO("Disconnecting...");
        if (client_exiting || exit_code != WHIST_EXIT_SUCCESS ||
            try_amount + 1 == MAX_INIT_CONNECTION_ATTEMPTS) {
            send_server_quit_messages(3);
        }

        // Destroy the network system
        destroy_packet_synchronizers();
        destroy_out_of_window_drag_handlers();
        destroy_file_synchronizer();
        destroy_clipboard_synchronizer();
        close_connections();

        // Destroy the renderer
        destroy_renderer(whist_renderer);

        // Mark as disconnected
        connected = false;
    }

#if USE_NEW_AUDIO_PATH
    quit_dedicated_audio_render_thread();
    whist_wait_thread(dedicated_audio_render_thread, NULL);
#endif

    if (exit_code != WHIST_EXIT_SUCCESS) {
        if (exit_code == WHIST_EXIT_FAILURE) {
            LOG_ERROR("Failure in main loop! Exiting with code WHIST_EXIT_FAILURE");
        } else if (exit_code == WHIST_EXIT_CLI) {
            // If we're in prod or staging, CLI errors are serious errors since we should always
            // be passing the correct arguments, so we log them as errors to report to Sentry.
            char* environment = get_error_monitor_environment();
            if (strcmp(environment, "prod") == 0 || strcmp(environment, "staging") == 0) {
                LOG_ERROR("Failure in main loop! Exiting with code WHIST_EXIT_CLI");
            } else {
                // In dev or localdev, CLI errors will happen a lot as engineers develop, so we only
                // log them as warnings
                LOG_WARNING("Failure in main loop! Exiting with code WHIST_EXIT_CLI");
            }
        } else {
            LOG_ERROR("Failure in main loop! Unhandled exit with unknown exit code: %d", exit_code);
        }
    }

    if (try_amount >= 3) {
        // we make this a LOG_WARNING so it doesn't clog up Sentry, as this
        // error happens periodically but we have recovery systems in place
        // for streaming interruption/connection loss
        LOG_WARNING("Failed to connect after three attempts!");
    }

    // Destroy any resources being used by the client
    LOG_INFO("Closing Client...");
    if (window) {
        destroy_sdl((SDL_Window*)window, frontend);
    }

    destroy_statistic_logger();

    LOG_INFO("Protocol has shutdown gracefully");

    destroy_logger();

    // We must call this after destroying the logger so that all
    // error monitor breadcrumbs and events can finish being reported
    // before we close the error monitor.
    whist_error_monitor_shutdown();

    LOG_INFO("Logger has shutdown gracefully");

    if (try_amount >= 3) {
        // We failed to connect, so return a failure error code
        return WHIST_EXIT_FAILURE;
    } else {
        return exit_code;
    }
}
