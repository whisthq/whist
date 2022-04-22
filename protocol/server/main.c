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
#include "dbus.h"

/*
============================
Defines
============================
*/
#ifndef LOG_CPU_USAGE
#define LOG_CPU_USAGE 0
#endif

#ifdef _WIN32
#define INPUT_TYPE WHIST_INPUT_DEVICE_WIN32
#else
#define INPUT_TYPE WHIST_INPUT_DEVICE_UINPUT
#endif

/*
============================
Globals
============================
*/

// TODO: Remove ugly global state
WhistServerState server_state;

// TODO: Remove, needed by udp.c temporarily
volatile bool connected = false;

/*
============================
Private Functions
============================
*/

void graceful_exit(WhistServerState* state);
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

void graceful_exit(WhistServerState* state) {
    /*
        Quit clients gracefully and allow server to exit.
    */

    // Mark server as an exiting state
    state->exiting = true;

    // If the client happens to be active, send a quit message
    ClientLock* client_lock = client_active_trylock(state->client);
    if (client_lock != NULL) {
        // Broadcast client quit message
        WhistServerMessage wsmsg_quit = {0};
        wsmsg_quit.type = SMESSAGE_QUIT;
        if (send_packet(&state->client->udp_context, PACKET_MESSAGE, &wsmsg_quit,
                        sizeof(WhistServerMessage), 1, false) < 0) {
            LOG_ERROR("Failed to send QUIT message");
        } else {
            LOG_INFO("Send QUIT message");
        }
        client_active_unlock(client_lock);
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

// Gets all pending Whist UDP messages
static void get_whist_udp_client_messages(WhistServerState* state) {
    WhistClientMessage wcmsg;
    size_t wcmsg_size;

    // If received a UDP message
    if (try_get_next_message_udp(state->client, &wcmsg, &wcmsg_size) == 0 && wcmsg_size != 0) {
        handle_client_message(state, &wcmsg);
    }
}

// Gets all pending Whist TCP messages
static bool get_whist_tcp_client_messages(WhistServerState* state) {
    bool ret = false;

    WhistPacket* tcp_packet = NULL;
    try_get_next_message_tcp(state->client, &tcp_packet);
    // If we get a TCP client message, handle it
    if (tcp_packet) {
        WhistClientMessage* wcmsg = (WhistClientMessage*)tcp_packet->data;
        LOG_INFO("TCP Packet type: %d", wcmsg->type);
        handle_client_message(state, wcmsg);
        free_packet(&state->client->tcp_context, tcp_packet);
        ret = true;
    }

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
    if (broadcast_tcp_packet(server_state.client, PACKET_MESSAGE, (uint8_t*)wmsg_tcp,
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
    WhistServerState* state = (WhistServerState*)opaque;

    LOG_INFO("multithreaded_sync_tcp_packets running on Thread %lu", whist_get_thread_id(NULL));

    init_clipboard_synchronizer(false);
    init_file_synchronizer((FILE_TRANSFER_SERVER_DROP | FILE_TRANSFER_SERVER_UPLOAD));

    // Hold a client active lock
    ClientLock* client_lock = client_active_lock(state->client);

    // TCP Message handler loop
    while (client_lock != NULL) {
        // Refresh the client activation lock, to let the client (re/de)activate if it's trying to
        client_active_unlock(client_lock);
        client_lock = client_active_lock(state->client);
        if (client_lock == NULL) {
            break;
        }

        // RECEIVE TCP PACKET HANDLER
        bool data_transferred = get_whist_tcp_client_messages(state);

        // SEND TCP PACKET HANDLERS:

        // GET CLIPBOARD HANDLER
        // If the clipboard has a new available chunk, we should send it over to the
        // client
        ClipboardData* clipboard_chunk = pull_clipboard_chunk();
        if (clipboard_chunk) {
            LOG_INFO("Received clipboard trigger. Broadcasting clipboard message.");
            create_and_send_tcp_wmsg(SMESSAGE_CLIPBOARD, (char*)clipboard_chunk);
            data_transferred = true;
            // Free clipboard chunk
            deallocate_region(clipboard_chunk);
        }

        // READ FILE CHUNK HANDLER
        FileData* file_chunk;
        FileMetadata* file_metadata;
        LinkedList* transferring_files = file_synchronizer_get_transferring_files();
        // Iterate through all file indexes and try to read next chunk to send
        linked_list_for_each(transferring_files, TransferringFile, transferring_file) {
            if (file_synchronizer_handle_type_group_end(transferring_file, NULL)) {
                // Returns true when the TransferringFile was a type group end indicator
                continue;
            }

            file_synchronizer_read_next_file_chunk(transferring_file, &file_chunk);
            if (file_chunk == NULL) {
                // If chunk cannot be read, then try opening the file
                file_synchronizer_open_file_for_reading(transferring_file, &file_metadata);
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

        // TODO: This should be set to a TCP socket timeout, not a manual sleep
        if (!data_transferred) whist_usleep(100);
    }

    // If we're holding a client lock, unlock it
    if (client_lock != NULL) {
        client_active_unlock(client_lock);
    }
    destroy_clipboard_synchronizer();
    destroy_file_synchronizer();

    return 0;
}

static void whist_server_state_init(WhistServerState* state, whist_server_config* config) {
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

    srand((unsigned int)time(NULL));
    server_state.connection_id = rand();

    server_state.input_device = create_input_device(INPUT_TYPE, NULL);
    if (!server_state.input_device) {
        LOG_FATAL("Failed to create input device.");
    }

    server_state.client = init_client();
}

int main(int argc, char* argv[]) {
    whist_server_config config = {0};

    int ret = server_parse_args(&config, argc, argv);
    if (ret == -1) {
        // invalid usage
        return -1;
    } else if (ret == 1) {
        // --help or --version
        return 0;
    }

    whist_init_subsystems();

    whist_init_statistic_logger(STATISTICS_FREQUENCY_IN_SEC);

    whist_server_state_init(&server_state, &config);

    // Initialize the error monitor, and tell it we are the server.
    whist_error_monitor_initialize(false);

    LOG_INFO("Whist server revision %s", whist_git_revision());
    LOG_INFO("Server protocol started.");

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

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
    server_state.update_encoder = false;
    server_state.exiting = false;

    server_state.ltr_context = ltr_create();
    server_state.stream_needs_restart = false;
    server_state.stream_needs_recovery = false;
    server_state.update_frame_ack = false;

    WhistThread send_video_thread =
        whist_create_thread(multithreaded_send_video, "multithreaded_send_video", &server_state);
    WhistThread send_audio_thread =
        whist_create_thread(multithreaded_send_audio, "multithreaded_send_audio", &server_state);

    DBusHandlerContext* dbus_handler = whist_create_dbus_handler(&server_state);

    WhistThread sync_tcp_packets_thread = whist_create_thread(
        multithreaded_sync_tcp_packets, "multithreaded_sync_tcp_packets", &server_state);
    LOG_INFO("Sending video, audio, and D-Bus handler...");

    WhistTimer totaltime;
    start_timer(&totaltime);

    LOG_INFO("Receiving packets...");

    init_window_info_getter();

    WhistTimer window_name_timer;
    start_timer(&window_name_timer);

    WhistTimer window_fullscreen_timer;
    start_timer(&window_fullscreen_timer);

    WhistTimer cpu_usage_statistics_timer;
    start_timer(&cpu_usage_statistics_timer);

#ifndef _WIN32
    WhistTimer uri_handler_timer;
    start_timer(&uri_handler_timer);

    WhistTimer downloaded_file_timer;
    start_timer(&downloaded_file_timer);

    WhistTimer uploaded_file_timer;
    start_timer(&uploaded_file_timer);
#endif  // ! _WIN32

    // Tracking how long we're willing to attempt to connect
    WhistTimer connection_attempt_timer;
    start_timer(&connection_attempt_timer);

    // Whether or not the client connected at least once
    bool client_connected_once = false;

    while (!server_state.exiting) {
        // Client management code

        // If the client isn't active, activate the client
        if (!server_state.client->is_active) {
            // If begin_time_to_exit seconds pass, and no connection has succeeded,
            if (config.begin_time_to_exit != -1 &&
                get_timer(&connection_attempt_timer) > config.begin_time_to_exit) {
                // Just exit
                server_state.exiting = true;
                break;
            }
            // Otherwise, try to connect
            if (connect_client(&server_state)) {
                // Mark the client as activated
                activate_client(server_state.client);
                client_connected_once = true;
            } else {
                LOG_INFO("No client found.");
            }
            continue;
        }

        // If the client is deactivating for any reason,
        if (server_state.client->is_deactivating) {
            // Wait for the client to deactivate (Which waits for every clientlock to be unlocked)
            deactivate_client(server_state.client);
            // Disconnect the client
            // TODO: Make this a function

            // Client and server share file transfer indexes when sending files, so
            //     when a client disconnects, we need to reset the transferring files
            //     to make sure that if a client reconnects that the indices are fresh.
            reset_all_transferring_files();

            // Destroy the udp/tcp socket contexts
            destroy_socket_context(&server_state.client->udp_context);
            destroy_socket_context(&server_state.client->tcp_context);

            // Restart the timer for the next connection attempt
            start_timer(&connection_attempt_timer);
            continue;
        }

        // UDP client messages, and various periodic updates

        // Get UDP messages
        get_whist_udp_client_messages(&server_state);

        // Log cpu usage once per second. Only enable this when LOG_CPU_USAGE flag is set because
        // getting cpu usage statistics is expensive.
        // TODO : Do NOT enable this, as it is bottlenecking UDP receive. Change expensive 'top'
        // based logic to simple '/proc/stat' logic for CPU usage.
        if (LOG_CPU_USAGE && get_timer(&cpu_usage_statistics_timer) > 1) {
            double cpu_usage = get_cpu_usage();
            log_double_statistic(SERVER_CPU_USAGE, cpu_usage);
            start_timer(&cpu_usage_statistics_timer);
        }

        if (get_timer(&window_fullscreen_timer) > 50.0 / MS_IN_SECOND) {
            // This is the cached fullscreen state. We only send state change events
            // to the client if the fullscreen value has changed.
            // TODO: Move static variable into client variable, so that it can clear on reactivation
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
                if (broadcast_tcp_packet(server_state.client, PACKET_MESSAGE, &wsmsg,
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
            if (name != NULL &&
                (server_state.client_joined_after_window_name_broadcast || new_window_name)) {
                LOG_INFO("%sBroadcasting window title message.",
                         new_window_name ? "Window title changed. " : "");
                static char wsmsg_buf[sizeof(WhistServerMessage) + WINDOW_NAME_MAXLEN + 1];
                WhistServerMessage* wsmsg = (void*)wsmsg_buf;
                wsmsg->type = SMESSAGE_WINDOW_TITLE;
                strncpy(wsmsg->window_title, name, WINDOW_NAME_MAXLEN + 1);
                if (broadcast_tcp_packet(server_state.client, PACKET_MESSAGE, (uint8_t*)wsmsg,
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
                    if (broadcast_tcp_packet(server_state.client, PACKET_MESSAGE, (uint8_t*)wsmsg,
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
                    file_synchronizer_end_type_group(FILE_TRANSFER_CLIENT_DOWNLOAD);
                    free(filename);
                } else {
                    LOG_WARNING("Unable to read downloaded file trigger file: %d", errno);
                }
                close(fd);
                remove(FILE_DOWNLOAD_TRIGGER_FILE);
            }

            start_timer(&downloaded_file_timer);
        }

#define FILE_UPLOAD_TRIGGER_FILE "/home/whist/.teleport/uploaded-file"
        if (get_timer(&uploaded_file_timer) > 50.0 / MS_IN_SECOND) {
            if (!access(FILE_UPLOAD_TRIGGER_FILE, R_OK)) {
                // If trigger file exists, request upload from client then delete the file
                WhistServerMessage wsmsg = {0};
                wsmsg.type = SMESSAGE_INITIATE_UPLOAD;
                if (broadcast_tcp_packet(server_state.client, PACKET_MESSAGE, &wsmsg,
                                         sizeof(WhistServerMessage)) == 0) {
                    LOG_INFO("Sent initiate upload message!");
                } else {
                    LOG_ERROR("Failed to broadcast initiate upload message.");
                }
                remove(FILE_UPLOAD_TRIGGER_FILE);
            }
            start_timer(&uploaded_file_timer);
        }

#endif  // ! _WIN32
    }

    if (!client_connected_once) {
        // Log to sentry that the client never ended up connecting
        LOG_ERROR("Client did not connect!");
    }

    // If the client is still active,
    if (server_state.client->is_active) {
        // Begin deactivating the client, if not done already
        start_deactivating_client(server_state.client);
        // Wait for the client to deactivate (Which waits for every clientlock to be unlocked)
        deactivate_client(server_state.client);
    }

    // Mark the client as permanently deactivated, which lets the threads reap
    permanently_deactivate_client(server_state.client);

    destroy_input_device(server_state.input_device);
    server_state.input_device = NULL;

    destroy_window_info_getter();

    whist_wait_thread(send_video_thread, NULL);
    whist_wait_thread(send_audio_thread, NULL);
    whist_wait_thread(sync_tcp_packets_thread, NULL);
    whist_destroy_dbus_handler(dbus_handler);

    ltr_destroy(server_state.ltr_context);

    LOG_INFO("Protocol has shutdown gracefully");

    destroy_statistic_logger();
    destroy_logger();
    whist_error_monitor_shutdown();

    // This is safe to call here because all other threads have been waited and destroyed
    destroy_client(server_state.client);

    return 0;
}
