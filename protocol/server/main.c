/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include "state.h"
#include "parse_args.h"
#include "handle_client_message.h"
#include "server_statistic.h"
#include "notifications.h"

/*
============================
Globals
============================
*/

// TODO: Remove ugly global state
whist_server_state server_state;

// TODO: Remove, needed by udp.c temporarily
volatile bool connected = false;

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
    WhistServerMessage wsmsg_response = {0};
    wsmsg_response.type = SMESSAGE_QUIT;
    if (state->client.is_active) {
        if (broadcast_udp_packet(&state->client, PACKET_MESSAGE, (uint8_t*)&wsmsg_response,
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

static void handle_whist_client_message(whist_server_state* state, WhistClientMessage* wcmsg) {
    /*
        Handles a Whist client message

        Arguments:
            wcmsg (WhistClientMessage*): the client message being handled
            id (int): the client ID
    */

    if (handle_client_message(state, wcmsg) != 0) {
        LOG_ERROR("Failed to handle message from client.");
    }
}

// Gets all pending Whist UDP messages
static void get_whist_udp_client_messages(whist_server_state* state) {
    if (!state->client.is_active) {
        return;
    }

    WhistClientMessage wcmsg;
    size_t wcmsg_size;

    // If received a UDP message
    if (try_get_next_message_udp(&state->client, &wcmsg, &wcmsg_size) == 0 && wcmsg_size != 0) {
        handle_whist_client_message(state, &wcmsg);
    }
}

// Gets all pending Whist TCP messages
static bool get_whist_tcp_client_messages(whist_server_state* state) {
    bool ret = false;
    if (!state->client.is_active) {
        return ret;
    }

    read_lock(&state->client.tcp_rwlock);

    WhistPacket* tcp_packet = NULL;
    try_get_next_message_tcp(&state->client, &tcp_packet);
    // If we get a TCP client message, handle it
    if (tcp_packet) {
        WhistClientMessage* wcmsg = (WhistClientMessage*)tcp_packet->data;
        LOG_INFO("TCP Packet type: %d", wcmsg->type);
        handle_whist_client_message(state, wcmsg);
        free_packet(&state->client.tcp_context, tcp_packet);
        ret = true;
    }

    read_unlock(&state->client.tcp_rwlock);
    return ret;
}

static void create_and_send_tcp_wmsg(WhistServerMessageType message_type, char* payload) {
    /*
        Create and send a TCP wmsg according to the given payload, and then
        deallocate once finished.

        Arguments:
            message_type (WhistServerMessageType): the type of the TCP message to be sent
            payload (char*): the payload of the TCP message
    */

    int data_size = 0;
    char* copy_location = NULL;
    int type_size = 0;

    switch (message_type) {
        case SMESSAGE_CLIPBOARD: {
            data_size = ((ClipboardData*)payload)->size;
            type_size = sizeof(ClipboardData);
            break;
        }
        case SMESSAGE_FILE_METADATA: {
            data_size = (int)((FileMetadata*)payload)->filename_len;
            type_size = sizeof(FileMetadata);
            break;
        }
        case SMESSAGE_FILE_DATA: {
            data_size = (int)((FileData*)payload)->size;
            type_size = sizeof(FileData);
            break;
        }
        default: {
            LOG_ERROR("Not a valid server wmsg type");
            return;
        }
    }

    // Alloc wmsg
    WhistServerMessage* wmsg_tcp = allocate_region(sizeof(WhistServerMessage) + data_size);

    switch (message_type) {
        case SMESSAGE_CLIPBOARD: {
            copy_location = (char*)&wmsg_tcp->clipboard;
            break;
        }
        case SMESSAGE_FILE_METADATA: {
            copy_location = (char*)&wmsg_tcp->file_metadata;
            break;
        }
        case SMESSAGE_FILE_DATA: {
            copy_location = (char*)&wmsg_tcp->file;
            break;
        }
        default: {
            // This is unreachable code, only here for consistency's sake
            deallocate_region(wmsg_tcp);
            LOG_ERROR("Not a valid server wmsg type");
            return;
        }
    }

    // Build wmsg
    // Init header to 0 to prevent sending uninitialized packets over the network
    memset(wmsg_tcp, 0, sizeof(*wmsg_tcp));
    wmsg_tcp->type = message_type;
    memcpy(copy_location, payload, type_size + data_size);
    // Send wmsg
    if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, (uint8_t*)wmsg_tcp,
                             sizeof(WhistServerMessage) + data_size) < 0) {
        LOG_WARNING("Failed to broadcast server message of type %d.", message_type);
    }
    // Free wmsg
    deallocate_region(wmsg_tcp);
}

static int multithreaded_sync_tcp_packets(void* opaque) {
    /*
        Thread to send and receive all TCP packets (clipboard and file)

        Arguments:
            opaque (void*): any arg to be passed to thread

        Return:
            (int): 0 on success
    */
    whist_server_state* state = (whist_server_state*)opaque;

    LOG_INFO("multithreaded_sync_tcp_packets running on Thread %lu", SDL_GetThreadID(NULL));

    init_clipboard_synchronizer(false);
    init_file_synchronizer(FILE_TRANSFER_SERVER_DROP);

    add_thread_to_client_active_dependents();
    bool assuming_client_active = false;
    while (!state->exiting) {
        update_client_active_status(&state->client, &assuming_client_active);

        // RECEIVE TCP PACKET HANDLER
        bool data_transferred = get_whist_tcp_client_messages(state);

        // SEND TCP PACKET HANDLERS:

        // GET CLIPBOARD HANDLER
        // If the clipboard has a new available chunk, we should send it over to the
        // client
        ClipboardData* clipboard_chunk = pull_clipboard_chunk();
        if (clipboard_chunk) {
            if (assuming_client_active) {
                LOG_INFO("Received clipboard trigger. Broadcasting clipboard message.");
                create_and_send_tcp_wmsg(SMESSAGE_CLIPBOARD, (char*)clipboard_chunk);
                data_transferred = true;
            }
            // Free clipboard chunk
            deallocate_region(clipboard_chunk);
        }

        // READ FILE CHUNK HANDLER
        FileData* file_chunk;
        FileMetadata* file_metadata;
        // Iterate through all file indexes and try to read next chunk to send
        for (int file_index = 0; file_index < NUM_TRANSFERRING_FILES; file_index++) {
            file_synchronizer_read_next_file_chunk(file_index, &file_chunk);
            if (file_chunk == NULL) {
                // If chunk cannot be read, then try opening the file
                file_synchronizer_open_file_for_reading(file_index, &file_metadata);
                if (file_metadata == NULL) {
                    continue;
                }

                create_and_send_tcp_wmsg(SMESSAGE_FILE_METADATA, (char*)file_metadata);
                data_transferred = true;
                // Free file chunk
                deallocate_region(file_metadata);
                continue;
            }

            create_and_send_tcp_wmsg(SMESSAGE_FILE_DATA, (char*)file_chunk);
            data_transferred = true;
            // Free file chunk
            deallocate_region(file_chunk);
        }

        // Sleep for a small duration to avoid busy looping when idle
        // 100us is just a low enough number to not affect the latency significantly. And it is also
        // reduces CPU usage significantly.
        if (!data_transferred) whist_usleep(100);
    }

    destroy_clipboard_synchronizer();
    destroy_file_synchronizer();

    return 0;
}

static void whist_server_state_init(whist_server_state* state, whist_server_config* config) {
    memset(state, 0, sizeof(*state));
    state->config = config;
    state->client_os = WHIST_UNKNOWN_OS;
    state->input_device = NULL;
    state->client_joined_after_window_name_broadcast = false;

    // Set width and height to -1. Video thread will start once correct dimensions are received from
    // client
    state->client_width = -1;
    state->client_height = -1;
    state->client_dpi = -1;
    state->update_device = true;

    // Mark initial update encoder
    state->update_encoder = true;

    state->discovery_listen = INVALID_SOCKET;
    state->tcp_listen = INVALID_SOCKET;
    state->udp_listen = INVALID_SOCKET;
}

#ifdef _WIN32
#define INPUT_TYPE WHIST_INPUT_DEVICE_WIN32
#else
#define INPUT_TYPE WHIST_INPUT_DEVICE_UINPUT
#endif

int main(int argc, char* argv[]) {
    InputDeviceType input_type = INPUT_TYPE;
    whist_server_config config;

    whist_init_subsystems();

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
        LOG_FATAL("Failed to create input device.");
    }

    if (init_client(&server_state.client) != 0) {
        LOG_FATAL("Failed to initialize client object.");
    }

#ifdef __linux__
    struct sigaction sa = {0};
    sa.sa_handler = sig_handler;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        LOG_FATAL("Establishing SIGTERM signal handler failed.");
    }

    XSetIOErrorHandler(xioerror_handler);
#endif

    WhistTimer startup_time;
    start_timer(&startup_time);

    server_state.stop_streaming = false;
    server_state.wants_iframe = false;
    server_state.update_encoder = false;
    server_state.exiting = false;

    WhistThread send_video_thread =
        whist_create_thread(multithreaded_send_video, "multithreaded_send_video", &server_state);
    WhistThread send_audio_thread =
        whist_create_thread(multithreaded_send_audio, "multithreaded_send_audio", &server_state);
    WhistThread notifs_thread = whist_create_thread(
        listen_and_process_notifications, "listen_and_process_notifications", &server_state);

    WhistThread manage_clients_thread = whist_create_thread(
        multithreaded_manage_client, "multithreaded_manage_client", &server_state);

    WhistThread sync_tcp_packets_thread = whist_create_thread(
        multithreaded_sync_tcp_packets, "multithreaded_sync_tcp_packets", &server_state);
    LOG_INFO("Sending video, audio, and notifications...");

    WhistTimer totaltime;
    start_timer(&totaltime);

    LOG_INFO("Receiving packets...");

    init_window_info_getter();

    WhistTimer window_name_timer;
    start_timer(&window_name_timer);

    WhistTimer window_fullscreen_timer;
    start_timer(&window_fullscreen_timer);

#ifndef _WIN32
    WhistTimer uri_handler_timer;
    start_timer(&uri_handler_timer);

    WhistTimer downloaded_file_timer;
    start_timer(&downloaded_file_timer);
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

        if (get_timer(&window_fullscreen_timer) > 50.0 / MS_IN_SECOND) {
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
                WhistServerMessage wsmsg = {0};
                wsmsg.type = SMESSAGE_FULLSCREEN;
                wsmsg.fullscreen = (int)fullscreen;
                if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, &wsmsg,
                                         sizeof(WhistServerMessage)) == 0) {
                    LOG_INFO("Sent fullscreen message!");
                    cur_fullscreen = fullscreen;
                } else {
                    LOG_ERROR("Failed to broadcast fullscreen message.");
                }
            }
            start_timer(&window_fullscreen_timer);
        }

        if (get_timer(&window_name_timer) > 50.0 / MS_IN_SECOND) {
            char* name = NULL;
            bool new_window_name = get_focused_window_name(&name);
            if (name != NULL && (server_state.client_joined_after_window_name_broadcast ||
                                 (assuming_client_active && new_window_name))) {
                LOG_INFO("%sBroadcasting window title message.",
                         new_window_name ? "Window title changed. " : "");
                static char wsmsg_buf[sizeof(WhistServerMessage) + WINDOW_NAME_MAXLEN + 1];
                WhistServerMessage* wsmsg = (void*)wsmsg_buf;
                wsmsg->type = SMESSAGE_WINDOW_TITLE;
                strncpy(wsmsg->window_title, name, WINDOW_NAME_MAXLEN + 1);
                if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, (uint8_t*)wsmsg,
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
        if (get_timer(&uri_handler_timer) > 50.0 / MS_IN_SECOND) {
            if (!access(URI_HANDLER_FILE, R_OK)) {
                // If the handler file exists, read it and delete the file
                int fd = open(URI_HANDLER_FILE, O_RDONLY);
                char handled_uri[HANDLED_URI_MAXLEN + 1] = {0};
                ssize_t bytes = read(fd, &handled_uri, HANDLED_URI_MAXLEN);
                if (bytes > 0) {
                    size_t wsmsg_size = sizeof(WhistServerMessage) + bytes + 1;
                    WhistServerMessage* wsmsg = safe_malloc(wsmsg_size);
                    memset(wsmsg, 0, sizeof(*wsmsg));
                    wsmsg->type = SMESSAGE_OPEN_URI;
                    memcpy(&wsmsg->requested_uri, handled_uri, bytes + 1);
                    if (broadcast_tcp_packet(&server_state.client, PACKET_MESSAGE, (uint8_t*)wsmsg,
                                             (int)wsmsg_size) < 0) {
                        LOG_WARNING("Failed to broadcast open URI message.");
                    } else {
                        LOG_INFO("Sent open URI message!");
                    }
                    free(wsmsg);
                } else {
                    LOG_WARNING("Unable to read URI handler file: %d", errno);
                }
                close(fd);
                remove(URI_HANDLER_FILE);
            }

            start_timer(&uri_handler_timer);
        }

#define FILE_DOWNLOAD_TRIGGER_FILE "/home/whist/.teleport/downloaded-file"
#define FILE_DOWNLOADS_PREFIX "/home/whist/Downloads"
#define DOWNLOADED_FILENAME_MAXLEN 4096
        if (get_timer(&downloaded_file_timer) > 50.0 / MS_IN_SECOND) {
            if (!access(FILE_DOWNLOAD_TRIGGER_FILE, R_OK)) {
                // If the trigger file exists, read it and delete the file
                int fd = open(FILE_DOWNLOAD_TRIGGER_FILE, O_RDONLY);
                char downloaded_filename[DOWNLOADED_FILENAME_MAXLEN + 1] = {0};
                ssize_t bytes = read(fd, &downloaded_filename, DOWNLOADED_FILENAME_MAXLEN);
                if (bytes > 0 && strchr(downloaded_filename, '/') == NULL) {
                    // Begin the file transfer to the client if a file was triggered and
                    // the filename doesn't include a / (this prevents escalation outside the
                    // Downloads directory)
                    char* filename = safe_malloc(strlen(FILE_DOWNLOADS_PREFIX) + 1 +
                                                 strlen(downloaded_filename) + 1);
                    snprintf(filename,
                             strlen(FILE_DOWNLOADS_PREFIX) + 1 + strlen(downloaded_filename) + 1,
                             "%s/%s", FILE_DOWNLOADS_PREFIX, downloaded_filename);
                    file_synchronizer_set_file_reading_basic_metadata(
                        filename, FILE_TRANSFER_CLIENT_DOWNLOAD, NULL);
                    free(filename);
                } else {
                    LOG_WARNING("Unable to read downloaded file trigger file: %d", errno);
                }
                close(fd);
                remove(FILE_DOWNLOAD_TRIGGER_FILE);
            }

            start_timer(&downloaded_file_timer);
        }
#endif  // ! _WIN32
    }

    destroy_input_device(server_state.input_device);
    server_state.input_device = NULL;

    destroy_window_info_getter();

    whist_wait_thread(send_video_thread, NULL);
    whist_wait_thread(send_audio_thread, NULL);
    whist_wait_thread(sync_tcp_packets_thread, NULL);
    whist_wait_thread(manage_clients_thread, NULL);

    // This is safe to call here because all other threads have been waited and destroyed
    if (quit_client(&server_state.client) != 0) {
        LOG_ERROR("Failed to quit clients.");
    }

    LOG_INFO("Protocol has shutdown gracefully");

    destroy_statistic_logger();
    destroy_logger();
    whist_error_monitor_shutdown();
    destroy_clients(&server_state.client);

    // Clean up d-bus connection
    // dbus_close(dbus_context);
    // event_base_free(eb);

    return 0;
}
