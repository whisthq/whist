/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.c
 * @brief This file contains the main code that runs a Fractal server on a
Windows or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Fractal video streaming server being created and creating
its threads.
*/

/*
============================
Includes
============================
*/

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#include <shlwapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <signal.h>
#include <unistd.h>
#endif
#include <time.h>

#include <fractal/utils/window_name.h>
#include <fractal/core/fractalgetopt.h>
#include <fractal/core/fractal.h>
#include <fractal/input/input.h>
#include <fractal/network/network.h>
#include <fractal/utils/aes.h>
#include <fractal/utils/error_monitor.h>
#include <fractal/video/transfercapture.h>
#include "client.h"
#include "handle_client_message.h"
#include "network.h"
#include "video.h"
#include "audio.h"

#ifdef _WIN32
#include <fractal/utils/windows_utils.h>
#endif

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

#define TCP_CONNECTION_WAIT 5000
#define CLIENT_PING_TIMEOUT_SEC 3.0

// i: means --identifier MUST take an argument
#define OPTION_STRING "k:i:w:e:t:"

extern Client clients[MAX_NUM_CLIENTS];

static char binary_aes_private_key[16];
static char hex_aes_private_key[33];

// This variables should stay as arrays - we call sizeof() on them
static char identifier[FRACTAL_IDENTIFIER_MAXLEN + 1];
static char webserver_url[WEBSERVER_URL_MAXLEN + 1];

volatile int connection_id;
volatile bool exiting;

int sample_rate = -1;

volatile double max_mbps;
InputDevice* input_device = NULL;

static FractalMutex packet_mutex;

volatile bool wants_iframe;
volatile bool update_encoder;

static bool client_joined_after_window_name_broadcast = false;
static int begin_time_to_exit = 60;
// This variable should always be an array - we call sizeof()
static char cur_window_name[WINDOW_NAME_MAXLEN + 1] = {0};

/*
============================
Custom Types
============================
*/

// required_argument means --identifier MUST take an argument
const struct option cmd_options[] = {{"private-key", required_argument, NULL, 'k'},
                                     {"identifier", required_argument, NULL, 'i'},
                                     {"webserver", required_argument, NULL, 'w'},
                                     {"environment", required_argument, NULL, 'e'},
                                     {"timeout", required_argument, NULL, 't'},
                                     // these are standard for POSIX programs
                                     {"help", no_argument, NULL, FRACTAL_GETOPT_HELP_CHAR},
                                     {"version", no_argument, NULL, FRACTAL_GETOPT_VERSION_CHAR},
                                     // end with NULL-termination
                                     {0, 0, 0, 0}};

/*
============================
Private Functions
============================
*/

void graceful_exit();
#ifdef __linux__
int xioerror_handler(Display* d);
void sig_handler(int sig_num);
#endif
bool get_using_stun();
int do_discovery_handshake(SocketContext* context, int* client_id);
int multithreaded_manage_clients(void* opaque);
int parse_args(int argc, char* argv[]);
int main(int argc, char* argv[]);

/*
============================
Private Function Implementations
============================
*/

void graceful_exit() {
    /*
        Quit clients gracefully and allow server to exit.
    */

    exiting = true;

    //  Quit all clients. This means that there is a possibility
    //  of the quitClients() pipeline happening more than once
    //  because this error handler can be called multiple times.

    // POSSIBLY below locks are not necessary if we're quitting everything and dying anyway?

    // Broadcast client quit message
    FractalServerMessage fmsg_response = {0};
    fmsg_response.type = SMESSAGE_QUIT;
    read_lock(&is_active_rwlock);
    if (broadcast_udp_packet(PACKET_MESSAGE, (uint8_t*)&fmsg_response, sizeof(FractalServerMessage),
                             1, STARTING_BURST_BITRATE, NULL, NULL) != 0) {
        LOG_WARNING("Could not send Quit Message");
    }
    read_unlock(&is_active_rwlock);

    // Kick all clients
    write_lock(&is_active_rwlock);
    fractal_lock_mutex(state_lock);
    if (quit_clients() != 0) {
        LOG_ERROR("Failed to quit clients.");
    }
    fractal_unlock_mutex(state_lock);
    write_unlock(&is_active_rwlock);
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

    graceful_exit();

    return 0;
}

void sig_handler(int sig_num) {
    /*
        When the server receives a SIGTERM, gracefully exit.
    */

    if (sig_num == SIGTERM) {
        graceful_exit();
    }
}
#endif

bool get_using_stun() {
    // decide whether the server is using stun. TODO: pull this and arg parsing into its own file
    return false;
}

int do_discovery_handshake(SocketContext* context, int* client_id) {
    FractalPacket* tcp_packet;
    clock timer;
    start_timer(&timer);
    do {
        tcp_packet = read_tcp_packet(context, true);
        fractal_sleep(5);
    } while (tcp_packet == NULL && get_timer(timer) < CLIENT_PING_TIMEOUT_SEC);
    // Exit on null tcp packet, otherwise analyze the resulting FractalClientMessage
    if (tcp_packet == NULL) {
        LOG_WARNING("Did not receive discovery request from client.");
        closesocket(context->socket);
        return -1;
    }

    FractalClientMessage* fcmsg = (FractalClientMessage*)tcp_packet->data;
    int user_id = fcmsg->discoveryRequest.user_id;

    read_lock(&is_active_rwlock);
    bool found;
    int ret;
    if ((ret = try_find_client_id_by_user_id(user_id, &found, client_id)) != 0) {
        LOG_ERROR(
            "Failed to try to find client ID by user ID. "
            " (User ID: %s)",
            user_id);
    }
    if (ret == 0 && found) {
        read_unlock(&is_active_rwlock);
        write_lock(&is_active_rwlock);
        ret = quit_client(*client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to quit client. (ID: %d)", *client_id);
        }
        write_unlock(&is_active_rwlock);
    } else {
        ret = get_available_client_id(client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to find available client ID.");
            closesocket(context->socket);
        }
        read_unlock(&is_active_rwlock);
        if (ret != 0) {
            free_tcp_packet(tcp_packet);
            return -1;
        }
    }

    clients[*client_id].user_id = user_id;
    LOG_INFO("Found ID for client. (ID: %d)", *client_id);

    // TODO: Should check for is_controlling, but that happens after this function call
    handle_client_message(fcmsg, *client_id, true);
    // fcmsg points into tcp_packet, but after this point, we don't use either,
    // so here we free the tcp packet
    free_tcp_packet(tcp_packet);
    fcmsg = NULL;
    tcp_packet = NULL;

    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage* fsmsg = safe_malloc(fsmsg_size);
    memset(fsmsg, 0, sizeof(*fsmsg));
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage* reply_msg = (FractalDiscoveryReplyMessage*)fsmsg->discovery_reply;

    reply_msg->client_id = *client_id;
    reply_msg->UDP_port = clients[*client_id].UDP_port;
    reply_msg->TCP_port = clients[*client_id].TCP_port;

    // Set connection ID in error monitor.
    error_monitor_set_connection_id(connection_id);

    // Send connection ID to client
    reply_msg->connection_id = connection_id;
    reply_msg->audio_sample_rate = sample_rate;

    LOG_INFO("Sending discovery packet");
    LOG_INFO("Fsmsg size is %d", (int)fsmsg_size);
    if (send_tcp_packet(context, PACKET_MESSAGE, (uint8_t*)fsmsg, (int)fsmsg_size) < 0) {
        LOG_ERROR("Failed to send send discovery reply message.");
        closesocket(context->socket);
        free(fsmsg);
        return -1;
    }

    closesocket(context->socket);
    free(fsmsg);
    return 0;
}

int multithreaded_manage_clients(void* opaque) {
    UNUSED(opaque);

    SocketContext discovery_context;
    int client_id;

    clock last_update_timer;
    start_timer(&last_update_timer);

    connection_id = rand();

    double nongraceful_grace_period = 600.0;  // 10 min after nongraceful disconn to reconn
    bool first_client_connected = false;      // set to true once the first client has connected
    bool disable_timeout = false;
    if (begin_time_to_exit ==
        -1) {  // client has `begin_time_to_exit` seconds to connect when the server first goes up.
               // If the variable is -1, disable auto-exit.
        disable_timeout = true;
    }
    clock first_client_timer;  // start this now and then discard when first client has connected
    start_timer(&first_client_timer);

    while (!exiting) {
        read_lock(&is_active_rwlock);

        int saved_num_active_clients = num_active_clients;

        read_unlock(&is_active_rwlock);

        LOG_INFO("Num Active Clients %d", saved_num_active_clients);

        if (saved_num_active_clients == 0) {
            connection_id = rand();

            // container exit logic -
            //  * clients have connected before but now none are connected
            //  * no clients have connected in `begin_time_to_exit` secs of server being up
            // We don't place this in a lock because:
            //  * if the first client connects right on the threshold of begin_time_to_exit, it
            //  doesn't matter if we disconnect
            //  * if a new client connects right on the threshold of nongraceful_grace_period, it
            //  doesn't matter if we disconnect
            //  * if no clients are connected, it isn't possible for another client to nongracefully
            //  exit and reset the grace period timer
            if (!disable_timeout &&
                (first_client_connected || (get_timer(first_client_timer) > begin_time_to_exit)) &&
                (!client_exited_nongracefully ||
                 (get_timer(last_nongraceful_exit) > nongraceful_grace_period))) {
                exiting = true;
            }
        } else {
            // nongraceful client grace period has ended, but clients are
            //  connected still - we don't want server to exit yet
            if (client_exited_nongracefully &&
                get_timer(last_nongraceful_exit) > nongraceful_grace_period) {
                client_exited_nongracefully = false;
            }
        }

        if (create_tcp_context(&discovery_context, NULL, PORT_DISCOVERY, 1, TCP_CONNECTION_WAIT,
                               get_using_stun(), binary_aes_private_key) < 0) {
            continue;
        }

        if (do_discovery_handshake(&discovery_context, &client_id) != 0) {
            LOG_WARNING("Discovery handshake failed.");
            continue;
        }

        LOG_INFO("Discovery handshake succeeded. (ID: %d)", client_id);

        // Client is not in use so we don't need to worry about anyone else
        // touching it
        if (connect_client(client_id, get_using_stun(), binary_aes_private_key) != 0) {
            LOG_WARNING(
                "Failed to establish connection with client. "
                "(ID: %d)",
                client_id);
            continue;
        }

        write_lock(&is_active_rwlock);

        LOG_INFO("Client connected. (ID: %d)", client_id);

        // We probably need to lock these. Argh.
        if (host_id == -1) {
            host_id = client_id;
        }

        if (num_active_clients == 0) {
            // we have went from 0 clients to 1 client, so we have got our first client
            // this variable should never be set back to false after this
            first_client_connected = true;
        }
        num_active_clients++;
        client_joined_after_window_name_broadcast = true;
        /* Make everyone a controller */
        clients[client_id].is_controlling = true;
        num_controlling_clients++;
        // if (num_controlling_clients == 0) {
        //     clients[client_id].is_controlling = true;
        //     num_controlling_clients++;
        // }

        if (clients[client_id].is_controlling) {
            // Reset input system when a new input controller arrives
            reset_input();
        }

        // reapTimedOutClients is called within a writeLock(&is_active_rwlock) and therefore this
        // should as well
        //  reapTimedOutClients only ever writes client_exited_nongracefully as true. This thread
        //  only writes it as false.
        if (client_exited_nongracefully &&
            (get_timer(last_nongraceful_exit) > nongraceful_grace_period)) {
            client_exited_nongracefully = false;
        }

        start_timer(&(clients[client_id].last_ping));

        clients[client_id].is_active = true;

        write_unlock(&is_active_rwlock);
    }

    return 0;
}

int parse_args(int argc, char* argv[]) {
    // TODO: replace `server` with argv[0]
    const char* usage =
        "Usage: server [OPTION]... IP_ADDRESS\n"
        "Try 'server --help' for more information.\n";
    const char* usage_details =
        "Usage: server [OPTION]... IP_ADDRESS\n"
        "\n"
        "All arguments to both long and short options are mandatory.\n"
        // regular options should look nice, with 2-space indenting for multiple lines
        "  -k, --private-key=PK          Pass in the RSA Private Key as a\n"
        "                                  hexadecimal string. Defaults to\n"
        "                                  binary and hex default keys in\n"
        "                                  the protocol code\n"
        "  -i, --identifier=ID           Pass in the unique identifier for this\n"
        "                                  server as a hexadecimal string\n"
        "  -e, --environment=ENV         The environment the protocol is running in,\n"
        "                                  e.g production, staging, development. Default: none\n"
        "  -w, --webserver=WS_URL        Pass in the webserver url for this\n"
        "                                  server's requests\n"
        "  -t, --timeout=TIME            Tell the server to give up after TIME seconds. If TIME\n"
        "                                  is -1, disable auto exit completely. Default: 60\n"
        // special options should be indented further to the left
        "      --help     Display this help and exit\n"
        "      --version  Output version information and exit\n";
    memcpy((char*)&binary_aes_private_key, DEFAULT_BINARY_PRIVATE_KEY,
           sizeof(binary_aes_private_key));
    memcpy((char*)&hex_aes_private_key, DEFAULT_HEX_PRIVATE_KEY, sizeof(hex_aes_private_key));

    int opt;

    while (true) {
        opt = getopt_long(argc, argv, OPTION_STRING, cmd_options, NULL);
        if (opt != -1 && optarg && strlen(optarg) > FRACTAL_ARGS_MAXLEN) {
            printf("Option passed into %c is too long! Length of %zd when max is %d\n", opt,
                   strlen(optarg), FRACTAL_ARGS_MAXLEN);
            return -1;
        }
        errno = 0;
        switch (opt) {
            case 'k': {
                if (!read_hexadecimal_private_key(optarg, (char*)binary_aes_private_key,
                                                  (char*)hex_aes_private_key)) {
                    printf("Invalid hexadecimal string: %s\n", optarg);
                    printf("%s", usage);
                    return -1;
                }
                break;
            }
            case 'i': {
                printf("Identifier passed in: %s\n", optarg);
                if (!safe_strncpy(identifier, optarg, sizeof(identifier))) {
                    printf("Identifier passed in is too long! Has length %lu but max is %d.\n",
                           (unsigned long)strlen(optarg), FRACTAL_IDENTIFIER_MAXLEN);
                    return -1;
                }
                break;
            }
            case 'w': {
                printf("Webserver URL passed in: %s\n", optarg);
                if (!safe_strncpy(webserver_url, optarg, sizeof(webserver_url))) {
                    printf("Webserver url passed in is too long! Has length %lu but max is %d.\n",
                           (unsigned long)strlen(optarg), WEBSERVER_URL_MAXLEN);
                }
                break;
            }
            case 'e': {
                error_monitor_set_environment(optarg);
                break;
            }
            case 't': {
                printf("Timeout before autoexit passed in: %s\n", optarg);
                if (sscanf(optarg, "%d", &begin_time_to_exit) != 1 ||
                    (begin_time_to_exit <= 0 && begin_time_to_exit != -1)) {
                    printf("Timeout should be a positive integer or -1\n");
                }
                break;
            }
            case FRACTAL_GETOPT_HELP_CHAR: {
                printf("%s", usage_details);
                return 1;
            }
            case FRACTAL_GETOPT_VERSION_CHAR: {
                printf("Fractal client revision %s\n", fractal_git_revision());
                return 1;
            }
            default: {
                if (opt != -1) {
                    // illegal option
                    printf("%s", usage);
                    return -1;
                }
                break;
            }
        }
        if (opt == -1) {
            bool can_accept_nonoption_args = false;
            if (optind < argc && can_accept_nonoption_args) {
                // there's a valid non-option arg
                // Do stuff with argv[optind]
                ++optind;
            } else if (optind < argc && !can_accept_nonoption_args) {
                // incorrect usage
                printf("%s", usage);
                return -1;
            } else {
                // we're done
                break;
            }
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    fractal_init_multithreading();
    init_logger();

    int ret = parse_args(argc, argv);
    if (ret == -1) {
        // invalid usage
        return -1;
    } else if (ret == 1) {
        // --help or --version
        return 0;
    }

    LOG_INFO("Server protocol started.");

    // Initialize the error monitor, and tell it we are the server.
    error_monitor_initialize(false);

    init_networking();

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

    srand((unsigned int)time(NULL));
    connection_id = rand();

    LOG_INFO("Fractal server revision %s", fractal_git_revision());

// initialize the windows socket library if this is a windows client
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        LOG_FATAL("Failed to initialize Winsock with error code: %d.", WSAGetLastError());
    }
#endif

    input_device = create_input_device();
    if (!input_device) {
        LOG_FATAL("Failed to create input device for playback.");
    }

#ifdef _WIN32
    if (!init_desktop(input_device, "winlogonpassword")) {
        LOG_FATAL("Could not winlogon!");
    }
#endif

    if (init_clients() != 0) {
        LOG_FATAL("Failed to initialize client objects.");
    }

#ifdef __linux__
    signal(SIGTERM, sig_handler);
    XSetIOErrorHandler(xioerror_handler);
#endif

    update_server_status(false, webserver_url, identifier, hex_aes_private_key);

    clock startup_time;
    start_timer(&startup_time);

    max_mbps = STARTING_BITRATE;
    wants_iframe = false;
    update_encoder = false;
    exiting = false;

    FractalThread manage_clients_thread =
        fractal_create_thread(multithreaded_manage_clients, "MultithreadedManageClients", NULL);
    fractal_sleep(500);

    FractalThread send_video_thread = fractal_create_thread(send_video, "send_video", NULL);
    FractalThread send_audio_thread = fractal_create_thread(send_audio, "send_audio", NULL);
    LOG_INFO("Sending video and audio...");

    clock totaltime;
    start_timer(&totaltime);

    clock last_exit_check;
    start_timer(&last_exit_check);

    clock last_ping_check;
    start_timer(&last_ping_check);

    LOG_INFO("Receiving packets...");

    init_clipboard_synchronizer(false);
    init_window_name_getter();

    clock ack_timer;
    start_timer(&ack_timer);

    clock window_name_timer;
    start_timer(&window_name_timer);

    while (!exiting) {
        if (get_timer(ack_timer) > 5) {
            if (get_using_stun()) {
                // Broadcast ack
                read_lock(&is_active_rwlock);
                if (broadcast_ack() != 0) {
                    LOG_ERROR("Failed to broadcast acks.");
                }
                read_unlock(&is_active_rwlock);
            }
            update_server_status(num_controlling_clients > 0, webserver_url, identifier,
                                 hex_aes_private_key);
            start_timer(&ack_timer);
        }

        // If they clipboard as updated, we should send it over to the
        // client
        ClipboardData* clipboard = clipboard_synchronizer_get_new_clipboard();
        if (clipboard) {
            LOG_INFO("Received clipboard trigger. Broadcasting clipboard message.");
            // Alloc fmsg
            FractalServerMessage* fmsg_response =
                allocate_region(sizeof(FractalServerMessage) + clipboard->size);
            // Build fmsg
            memset(fmsg_response, 0, sizeof(*fmsg_response));
            fmsg_response->type = SMESSAGE_CLIPBOARD;
            memcpy(&fmsg_response->clipboard, clipboard, sizeof(ClipboardData) + clipboard->size);
            // Send fmsg
            read_lock(&is_active_rwlock);
            if (broadcast_tcp_packet(PACKET_MESSAGE, (uint8_t*)fmsg_response,
                                     sizeof(FractalServerMessage) + clipboard->size) < 0) {
                LOG_WARNING("Failed to broadcast clipboard message.");
            }
            read_unlock(&is_active_rwlock);
            // Free fmsg
            deallocate_region(fmsg_response);
            // Free clipboard
            free_clipboard(clipboard);
        }

        if (get_timer(window_name_timer) > 0.1) {  // poll window name every 100ms
            char name[WINDOW_NAME_MAXLEN + 1];
            if (get_focused_window_name(name) == 0) {
                if (client_joined_after_window_name_broadcast ||
                    (num_active_clients > 0 && strcmp(name, cur_window_name) != 0)) {
                    LOG_INFO("Window title changed. Broadcasting window title message.");
                    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(name);
                    FractalServerMessage* fmsg_response = safe_malloc(fsmsg_size);
                    memset(fmsg_response, 0, sizeof(*fmsg_response));
                    fmsg_response->type = SMESSAGE_WINDOW_TITLE;
                    memcpy(&fmsg_response->window_title, name, sizeof(name));
                    read_lock(&is_active_rwlock);
                    if (broadcast_tcp_packet(PACKET_MESSAGE, (uint8_t*)fmsg_response,
                                             (int)fsmsg_size) < 0) {
                        LOG_WARNING("Failed to broadcast window title message.");
                    } else {
                        LOG_INFO("Sent window title message!");
                        safe_strncpy(cur_window_name, name, sizeof(cur_window_name));
                        client_joined_after_window_name_broadcast = false;
                    }
                    read_unlock(&is_active_rwlock);
                    free(fmsg_response);
                }
            }
            start_timer(&window_name_timer);
        }

        if (get_timer(last_ping_check) > 20.0) {
            for (;;) {
                read_lock(&is_active_rwlock);
                bool exists, should_reap = false;
                if (exists_timed_out_client(CLIENT_PING_TIMEOUT_SEC, &exists) != 0) {
                    LOG_ERROR("Failed to find if a client has timed out.");
                } else {
                    should_reap = exists;
                }
                read_unlock(&is_active_rwlock);
                if (should_reap) {
                    write_lock(&is_active_rwlock);
                    if (reap_timed_out_clients(CLIENT_PING_TIMEOUT_SEC) != 0) {
                        LOG_ERROR("Failed to reap timed out clients.");
                    }
                    write_unlock(&is_active_rwlock);
                }
                break;
            }
            start_timer(&last_ping_check);
        }

        read_lock(&is_active_rwlock);

        for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
            if (!clients[id].is_active) continue;

            // Get packet!
            FractalPacket* tcp_packet = NULL;
            FractalClientMessage* fmsg = NULL;
            FractalClientMessage local_fcmsg;
            size_t fcmsg_size;
            if (try_get_next_message_tcp(id, &tcp_packet) != 0 || tcp_packet == NULL) {
                // On no TCP
                if (try_get_next_message_udp(id, &local_fcmsg, &fcmsg_size) != 0 ||
                    fcmsg_size == 0) {
                    // On no UDP
                    continue;
                }
                // On UDP
                fmsg = &local_fcmsg;
            } else {
                // On TCP
                fmsg = (FractalClientMessage*)tcp_packet->data;
            }

            // HANDLE FRACTAL CLIENT MESSAGE
            fractal_lock_mutex(state_lock);
            bool is_controlling = clients[id].is_controlling;
            fractal_unlock_mutex(state_lock);
            if (handle_client_message(fmsg, id, is_controlling) != 0) {
                LOG_ERROR(
                    "Failed to handle message from client. "
                    "(ID: %d)",
                    id);
            } else {
                // if (handleSpectatorMessage(fmsg, id) != 0) {
                //     LOG_ERROR("Failed to handle message from spectator");
                // }
            }

            // Free the tcp packet if we received one
            if (tcp_packet) {
                free_tcp_packet(tcp_packet);
            }
        }
        read_unlock(&is_active_rwlock);
    }

    destroy_input_device(input_device);
    destroy_clipboard_synchronizer();
    destroy_window_name_getter();

    fractal_wait_thread(send_video_thread, NULL);
    fractal_wait_thread(send_audio_thread, NULL);
    fractal_wait_thread(manage_clients_thread, NULL);

    fractal_destroy_mutex(packet_mutex);

    write_lock(&is_active_rwlock);
    fractal_lock_mutex(state_lock);
    if (quit_clients() != 0) {
        LOG_ERROR("Failed to quit clients.");
    }
    fractal_unlock_mutex(state_lock);
    write_unlock(&is_active_rwlock);

#ifdef _WIN32
    WSACleanup();
#endif

    destroy_logger();
    error_monitor_shutdown();
    destroy_clients();

    return 0;
}
