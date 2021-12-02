/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file main.c
 * @brief This file contains the main code that runs a Whist server on a
 *        Windows or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Whist video streaming server being created and creating
its threads.
*/

/*
============================
Includes
============================
*/

#include "main.h"
#include "parse_args.h"
#include "handle_client_message.h"
#include "server_statistic.h"

/*
============================
Globals
============================
*/

static WhistMutex packet_mutex;

// This variable should always be an array - we call sizeof()

whist_server_state server_state;

/*
============================
Private Functions
============================
*/

void graceful_exit(whist_server_state* state);
#ifdef __linux__
int xioerror_handler(Display* d);
void sig_handler(int sig_num);
#endif
int main(int argc, char* argv[]);

/*
============================
Private Function Implementations
============================
*/

void graceful_exit(whist_server_state* state) {
    /*
        Quit clients gracefully and allow server to exit.
    */

    state->exiting = true;

    //  Quit all clients. This means that there is a possibility
    //  of the quitClients() pipeline happening more than once
    //  because this error handler can be called multiple times.

    // POSSIBLY below locks are not necessary if we're quitting everything and dying anyway?

    // Broadcast client quit message
    WhistServerMessage fsmsg_response = {0};
    fsmsg_response.type = SMESSAGE_QUIT;
    if (state->client.is_active) {
        if (broadcast_udp_packet(&state->client, PACKET_MESSAGE, (uint8_t*)&fsmsg_response,
                                 sizeof(WhistServerMessage), 1) != 0) {
            LOG_WARNING("Could not send Quit Message");
        }
    }

    // Kick all clients
    if (start_quitting_client(&state->client) != 0) {
        LOG_ERROR("Failed to start quitting client.");
    }
}

#ifdef __linux__
int xioerror_handler(Display* d) {
    /*
        When X display is destroyed, intercept XIOError in order to
        quit clients.

        For use as arg for XSetIOErrorHandler - this is fatal and thus
        any program exit handling that would normally be expected to
        be handled in another thread must be explicitly handled here.
        Right now, we handle:
            * SendContainerDestroyMessage
            * quitClients
    */

    graceful_exit(&server_state);

    return 0;
}

void sig_handler(int sig_num) {
    /*
        When the server receives a SIGTERM, gracefully exit.
    */

    if (sig_num == SIGTERM) {
        graceful_exit(&server_state);
    }
}
#endif

void handle_whist_client_message(whist_server_state* state, WhistClientMessage* fcmsg) {
    /*
        Handles a Whist client message

        Arguments:
            fcmsg (WhistClientMessage*): the client message being handled
            id (int): the client ID
    */

    if (handle_client_message(state, fcmsg) != 0) {
        LOG_ERROR("Failed to handle message from client.");
    }
}

// Gets all pending Whist UDP messages
void get_whist_udp_client_messages(whist_server_state* state) {
    if (!state->client.is_active) {
        return;
    }

    WhistClientMessage fcmsg;
    size_t fcmsg_size;

    // If received a UDP message
    if (try_get_next_message_udp(&state->client, &fcmsg, &fcmsg_size) == 0 && fcmsg_size != 0) {
        handle_whist_client_message(state, &fcmsg);
    }
}

// Gets all pending Whist TCP messages
void get_whist_tcp_client_messages(whist_server_state* state) {
    if (!state->client.is_active) {
        return;
    }

    read_lock(&state->client.tcp_rwlock);

    WhistPacket* tcp_packet = NULL;
    try_get_next_message_tcp(&state->client, &tcp_packet);
    // If we get a TCP client message, handle it
    if (tcp_packet) {
        WhistClientMessage* fcmsg = (WhistClientMessage*)tcp_packet->data;
        LOG_INFO("TCP Packet type: %d", fcmsg->type);
        handle_whist_client_message(state, fcmsg);
        free_packet(&state->client.tcp_context, tcp_packet);
    }

    read_unlock(&state->client.tcp_rwlock);
}

int multithreaded_sync_tcp_packets(void* opaque) {
    /*
        Thread to send and receive all TCP packets (clipboard and file)

        Arguments:
            opaque (void*): any arg to be passed to thread

        Return:
            (int): 0 on success
    */
    whist_server_state* state = (whist_server_state*)opaque;

    LOG_INFO("multithreaded_sync_tcp_packets running on Thread %p", SDL_GetThreadID(NULL));

    // TODO: compartmentalize each part into its own function
    init_clipboard_synchronizer(false);

    add_thread_to_client_active_dependents();
    bool assuming_client_active = false;
    while (!state->exiting) {
        update_client_active_status(&state->client, &assuming_client_active);

        // RECEIVE TCP PACKET HANDLER
        get_whist_tcp_client_messages(state);

        // SEND TCP PACKET HANDLERS:

        // GET CLIPBOARD HANDLER
        // If the clipboard has a new available chunk, we should send it over to the
        // client
        ClipboardData* clipboard_chunk = pull_clipboard_chunk();
        if (clipboard_chunk) {
            if (assuming_client_active) {
                LOG_INFO("Received clipboard trigger. Broadcasting clipboard message.");
                // Alloc fsmsg
                WhistServerMessage* fsmsg_response =
                    allocate_region(sizeof(WhistServerMessage) + clipboard_chunk->size);
                // Build fsmsg
                memset(fsmsg_response, 0, sizeof(*fsmsg_response));
                fsmsg_response->type = SMESSAGE_CLIPBOARD;
                memcpy(&fsmsg_response->clipboard, clipboard_chunk,
                       sizeof(ClipboardData) + clipboard_chunk->size);
                // Send fsmsg
                if (broadcast_tcp_packet(&state->client, PACKET_MESSAGE, (uint8_t*)fsmsg_response,
                                         sizeof(WhistServerMessage) + clipboard_chunk->size) < 0) {
                    LOG_WARNING("Failed to broadcast clipboard message.");
                }
                // Free fsmsg
                deallocate_region(fsmsg_response);
            }
            // Free clipboard chunk
            deallocate_region(clipboard_chunk);
        }

        // TODO: add file send handler
    }

    destroy_clipboard_synchronizer();

    return 0;
}

static void whist_server_state_init(whist_server_state* state, whist_server_config* config) {
    memset(state, 0, sizeof(*state));
    state->config = config;
    state->client_os = WHIST_UNKNOWN_OS;
    state->sample_rate = -1;
    state->max_bitrate = STARTING_BITRATE;
    state->input_device = NULL;
    state->client_joined_after_window_name_broadcast = false;

    state->client_width = -1;
    state->client_height = -1;
    state->client_dpi = -1;
    state->client_codec_type = CODEC_TYPE_UNKNOWN;
    state->update_device = true;

    state->discovery_listen = INVALID_SOCKET;
    state->tcp_listen = INVALID_SOCKET;
    state->udp_listen = INVALID_SOCKET;
}

int main(int argc, char* argv[]) {
    whist_server_config config;

    whist_init_multithreading();

    whist_init_networking();
    whist_init_logger();
    whist_init_server_statistics();
    whist_init_statistic_logger(SERVER_NUM_METRICS, server_statistic_info,
                                STATISTICS_FREQUENCY_IN_SEC);

    int ret = server_parse_args(&config, argc, argv);
    if (ret == -1) {
        // invalid usage
        return -1;
    } else if (ret == 1) {
        // --help or --version
        return 0;
    }

    whist_server_state_init(&server_state, &config);

    LOG_INFO("Server protocol started.");

    // Initialize the error monitor, and tell it we are the server.
    whist_error_monitor_initialize(false);

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

    srand((unsigned int)time(NULL));
    server_state.connection_id = rand();

    LOG_INFO("Whist server revision %s", whist_git_revision());

    server_state.input_device = create_input_device();
    if (!server_state.input_device) {
        LOG_FATAL("Failed to create input device for playback.");
    }

    if (init_client(&server_state.client) != 0) {
        LOG_FATAL("Failed to initialize client object.");
    }

#ifdef __linux__
    signal(SIGTERM, sig_handler);
    XSetIOErrorHandler(xioerror_handler);
#endif

    clock startup_time;
    start_timer(&startup_time);

    server_state.max_bitrate = STARTING_BITRATE;
    server_state.stop_streaming = false;
    server_state.wants_iframe = false;
    server_state.update_encoder = false;
    server_state.exiting = false;

    WhistThread send_video_thread =
        whist_create_thread(multithreaded_send_video, "multithreaded_send_video", &server_state);
    WhistThread send_audio_thread =
        whist_create_thread(multithreaded_send_audio, "multithreaded_send_audio", &server_state);

    WhistThread manage_clients_thread = whist_create_thread(
        multithreaded_manage_client, "multithreaded_manage_client", &server_state);

    WhistThread sync_tcp_packets_thread = whist_create_thread(
        multithreaded_sync_tcp_packets, "multithreaded_sync_tcp_packets", &server_state);
    LOG_INFO("Sending video and audio...");

    clock totaltime;
    start_timer(&totaltime);

    LOG_INFO("Receiving packets...");

    init_window_info_getter();

    clock ack_timer;
    start_timer(&ack_timer);

    clock window_name_timer;
    start_timer(&window_name_timer);

    clock window_fullscreen_timer;
    start_timer(&window_fullscreen_timer);

#ifndef _WIN32
    clock uri_handler_timer;
    start_timer(&uri_handler_timer);
#endif  // ! _WIN32

    add_thread_to_client_active_dependents();
    bool assuming_client_active = false;
    while (!server_state.exiting) {
        update_client_active_status(&server_state.client, &assuming_client_active);

        if (!assuming_client_active) {
            continue;
        }

        // Get UDP messages
        get_whist_udp_client_messages(&server_state);

        if (get_timer(ack_timer) > 5) {
            if (get_using_stun()) {
                // Broadcast ack
                if (broadcast_ack(&server_state.client) != 0) {
                    LOG_ERROR("Failed to broadcast acks.");
                }
            }
            start_timer(&ack_timer);
        }

        if (get_timer(window_fullscreen_timer) > 50.0 / MS_IN_SECOND) {
            // This is the cached fullscreen state. We only send state change events
            // to the client if the fullscreen value has changed.
            static bool cur_fullscreen = false;
            bool fullscreen = is_focused_window_fullscreen();
            if (fullscreen != cur_fullscreen) {
                if (fullscreen) {
                    LOG_INFO("Window is now fullscreen. Broadcasting fullscreen message.");
                } else {
                    LOG_INFO("Window is no longer fullscreen. Broadcasting fullscreen message.");
                }
                WhistServerMessage fsmsg = {0};
                fsmsg.type = SMESSAGE_FULLSCREEN;
                fsmsg.fullscreen = (int)fullscreen;
                if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, &fsmsg,
                                         sizeof(WhistServerMessage)) == 0) {
                    LOG_INFO("Sent fullscreen message!");
                    cur_fullscreen = fullscreen;
                } else {
                    LOG_ERROR("Failed to broadcast fullscreen message.");
                }
            }
            start_timer(&window_fullscreen_timer);
        }

        if (get_timer(window_name_timer) > 50.0 / MS_IN_SECOND) {
            char* name = NULL;
            bool new_window_name = get_focused_window_name(&name);
            if (name != NULL && (server_state.client_joined_after_window_name_broadcast ||
                                 (assuming_client_active && new_window_name))) {
                LOG_INFO("%sBroadcasting window title message.",
                         new_window_name ? "Window title changed. " : "");
                static char fsmsg_buf[sizeof(WhistServerMessage) + WINDOW_NAME_MAXLEN + 1];
                WhistServerMessage* fsmsg = (void*)fsmsg_buf;
                fsmsg->type = SMESSAGE_WINDOW_TITLE;
                strncpy(fsmsg->window_title, name, WINDOW_NAME_MAXLEN + 1);
                if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, (uint8_t*)fsmsg,
                                         (int)(sizeof(WhistServerMessage) + strlen(name) + 1)) ==
                    0) {
                    LOG_INFO("Sent window title message!");
                    server_state.client_joined_after_window_name_broadcast = false;
                } else {
                    LOG_WARNING("Failed to broadcast window title message.");
                }
            }
            start_timer(&window_name_timer);
        }

#ifndef _WIN32
#define URI_HANDLER_FILE "/home/whist/.teleport/handled-uri"
#define HANDLED_URI_MAXLEN 4096
        if (get_timer(uri_handler_timer) > 50.0 / MS_IN_SECOND) {
            if (!access(URI_HANDLER_FILE, R_OK)) {
                // If the handler file exists, read it and delete the file
                int fd = open(URI_HANDLER_FILE, O_RDONLY);
                char handled_uri[HANDLED_URI_MAXLEN + 1] = {0};
                ssize_t bytes = read(fd, &handled_uri, HANDLED_URI_MAXLEN);
                if (bytes > 0) {
                    size_t fsmsg_size = sizeof(WhistServerMessage) + bytes + 1;
                    WhistServerMessage* fsmsg = safe_malloc(fsmsg_size);
                    memset(fsmsg, 0, sizeof(*fsmsg));
                    fsmsg->type = SMESSAGE_OPEN_URI;
                    memcpy(&fsmsg->requested_uri, handled_uri, bytes + 1);
                    if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, (uint8_t*)fsmsg,
                                             (int)fsmsg_size) < 0) {
                        LOG_WARNING("Failed to broadcast open URI message.");
                    } else {
                        LOG_INFO("Sent open URI message!");
                    }
                    free(fsmsg);
                } else {
                    LOG_WARNING("Unable to read URI handler file: %d", errno);
                }
                close(fd);
                remove(URI_HANDLER_FILE);
            }

            start_timer(&uri_handler_timer);
        }
#endif  // ! _WIN32
    }

    destroy_input_device(server_state.input_device);

    destroy_window_info_getter();

    whist_wait_thread(send_video_thread, NULL);
    whist_wait_thread(send_audio_thread, NULL);
    whist_wait_thread(sync_tcp_packets_thread, NULL);
    whist_wait_thread(manage_clients_thread, NULL);

    whist_destroy_mutex(packet_mutex);

    // This is safe to call here because all other threads have been waited and destroyed
    if (quit_client(&server_state.client) != 0) {
        LOG_ERROR("Failed to quit clients.");
    }

    LOG_INFO("Protocol has shutdown gracefully");

    destroy_statistic_logger();
    destroy_logger();
    error_monitor_shutdown();
    destroy_clients(&server_state.client);

    return 0;
}
