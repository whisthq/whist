/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sync_packets.c
 * @brief This file contains code for high-level communication with the server
============================
Usage
============================
Use multithreaded_sync_udp_packets to send and receive UDP messages to and from the server, such as
audio and video packets. Use multithreaded_sync_tcp_packets to send and receive TCP messages, mostly
clipboard packets.
*/

/*
============================
Includes
============================
*/
#include <whist/core/whist.h>
#include <whist/utils/clock.h>
#include <whist/network/network.h>
#include <whist/logging/log_statistic.h>
#include <whist/logging/logging.h>

#include "handle_server_message.h"
#include "network.h"
#include "audio.h"
#include "video.h"
#include "sync_packets.h"
#include "client_utils.h"
#include "client_statistic.h"
#include "notifications.h"

// Updater variables
extern SocketContext packet_udp_context;
extern SocketContext packet_tcp_context;
bool connected = true;  // The state of the client, i.e. whether it's connected to a server or not
// Ping variables
WhistTimer last_ping_timer;
volatile int last_udp_ping_id;
volatile int last_udp_pong_id;
volatile int udp_ping_failures;
// TCP ping variables
WhistTimer last_tcp_ping_timer;
volatile int last_tcp_ping_id;
volatile int last_tcp_pong_id;
extern volatile double latency;

// Threads
static WhistThread sync_udp_packets_thread;
static WhistThread sync_tcp_packets_thread;
static bool run_sync_packets_threads;

/*
============================
Private Function Implementations
============================
*/

void update_ping() {
    /*
       Check if we should send more pings, disconnect, etc. If no valid pong has been received for
       600ms, we mark that as a ping failure. If we successfully received a pong and it has been
       500ms since the last ping, we send the next ping. Otherwise, if we haven't yet received a
       pong and it has been 210 ms, resend the ping.
    */

    static WhistTimer last_new_ping_timer;
    static bool timer_initialized = false;
    if (!timer_initialized) {
        start_timer(&last_new_ping_timer);
        timer_initialized = true;
    }

    // If it's been 1 second since the last ping, we should warn
    if (get_timer(&last_ping_timer) > 1.0) {
        LOG_WARNING("No ping sent or pong received in over a second");
    }

    // If we're waiting for a ping, and it's been 600ms, then that ping will be
    // noted as failed
    if (last_udp_ping_id != last_udp_pong_id && get_timer(&last_new_ping_timer) > 0.6) {
        LOG_WARNING("Ping received no response: %d", last_udp_ping_id);
        // Keep track of failures, and exit if too many failures
        last_udp_pong_id = last_udp_ping_id;
        ++udp_ping_failures;
        if (udp_ping_failures == 3) {
            // we make this a LOG_WARNING so it doesn't clog up Sentry, as this
            // error happens periodically but we have recovery systems in place
            // for streaming interruption/connection loss
            LOG_WARNING("Server disconnected: 3 consecutive ping failures.");
            connected = false;
        }
    }

    // if we've received the last ping, send another
    if (last_udp_ping_id == last_udp_pong_id && get_timer(&last_ping_timer) > 0.5) {
        send_ping(last_udp_ping_id + 1);
        start_timer(&last_new_ping_timer);
    }

    // if we haven't received the last ping, send the same ping
    if (last_udp_ping_id != last_udp_pong_id && get_timer(&last_ping_timer) > 0.21) {
        send_ping(last_udp_ping_id);
    }
}

void update_tcp_ping() {
    /*
       If no valid TCP pong has been received or sending a TCP ping is failing, then
       send a TCP reconnection request to the server. This is agnostic of whether
       the lost connection was caused by the client or the server.
    */

    static WhistTimer last_new_ping_timer;
    static bool timer_initialized = false;
    if (!timer_initialized) {
        start_timer(&last_new_ping_timer);
        timer_initialized = true;
    }

    // If it's been 10 seconds since the last TCP ping, we should warn
    if (get_timer(&last_tcp_ping_timer) > 10.0) {
        LOG_WARNING("No TCP ping sent or pong received in over 10 seconds");
    }

    // If we're waiting for a ping, and it's been 5s, then that ping will be
    // noted as failed
    if (last_tcp_ping_id != last_tcp_pong_id && get_timer(&last_new_ping_timer) > 5.0) {
        LOG_WARNING("TCP ping received no response: %d", last_tcp_ping_id);

        // Only if we successfully recover the TCP connection should we continue
        //     as if the ping was successful.
        if (send_tcp_reconnect_message() == 0) {
            last_tcp_pong_id = last_tcp_ping_id;
        }
    }

    // If we've received the last pong, send another if it's been 2s since last ping
    if (last_tcp_ping_id == last_tcp_pong_id && get_timer(&last_tcp_ping_timer) > 2.0) {
        send_tcp_ping(last_tcp_ping_id + 1);
        start_timer(&last_new_ping_timer);
    }
}

/*
============================
Public Function Implementations
============================
*/
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
int multithreaded_sync_udp_packets(void* opaque) {
    /*
        Send, receive, and process UDP packets - dimension messages, bitrate messages, nack
       messages, pings, audio and video packets.
    */
    WhistRenderer* whist_renderer = (WhistRenderer*)opaque;
    SocketContext* socket_context = &packet_udp_context;

    // we initialize latency here because on macOS, latency would not initialize properly in
    // its global declaration. We start at 25ms before the first ping.
    latency = 25.0 / MS_IN_SECOND;
    last_udp_ping_id = 0;
    udp_ping_failures = 0;

    WhistTimer last_ack;
    WhistTimer statistics_timer;
    start_timer(&last_ack);

    while (run_sync_packets_threads) {
        // Ack the connection every 5 seconds
        if (get_timer(&last_ack) > 5.0) {
            ack(socket_context);
            start_timer(&last_ack);
        }

        update_ping();
        // Update the renderer
        renderer_update(whist_renderer);
        TIME_RUN(WhistPacket* packet = read_packet(socket_context, true), NETWORK_READ_PACKET_UDP,
                 statistics_timer);

        if (!packet) {
            continue;
        }

        switch (packet->type) {
            case PACKET_AUDIO:
            case PACKET_VIDEO: {
                // Pass the audio/video packet into the renderer
                renderer_receive_packet(whist_renderer, packet);
                break;
            }
            case PACKET_MESSAGE: {
                TIME_RUN(handle_server_message((WhistServerMessage*)packet->data,
                                               (size_t)packet->payload_size),
                         SERVER_HANDLE_MESSAGE_UDP, statistics_timer);
                break;
            }
            case PACKET_NOTIFICATION: {
                display_notification(packet);
                log_double_statistic(NOTIFICATIONS_RECEIVED, 1.);
                break;
            }
            default:
                LOG_ERROR("Unknown packet type: %d", packet->type);
                break;
        }
        free_packet(socket_context, packet);
    }
    return 0;
}

void create_and_send_tcp_wcmsg(WhistClientMessageType message_type, char* payload) {
    /*
        Create and send a TCP wcmsg according to the given payload, and then
        deallocate once finished.

        Arguments:
            message_type (WhistClientMessageType): the type of the TCP message to be sent
            payload (char*): the payload of the TCP message
    */

    int data_size = 0;
    char* copy_location = NULL;
    int type_size = 0;

    switch (message_type) {
        case CMESSAGE_CLIPBOARD: {
            data_size = ((ClipboardData*)payload)->size;
            type_size = sizeof(ClipboardData);
            break;
        }
        case CMESSAGE_FILE_METADATA: {
            data_size = (int)((FileMetadata*)payload)->filename_len;
            type_size = sizeof(FileMetadata);
            break;
        }
        case CMESSAGE_FILE_DATA: {
            data_size = (int)((FileData*)payload)->size;
            type_size = sizeof(FileData);
            break;
        }
        default: {
            LOG_ERROR("Not a valid server wcmsg type");
            return;
        }
    }

    // Alloc wcmsg
    WhistClientMessage* wcmsg_tcp = allocate_region(sizeof(WhistClientMessage) + data_size);

    switch (message_type) {
        case CMESSAGE_CLIPBOARD: {
            copy_location = (char*)&wcmsg_tcp->clipboard;
            break;
        }
        case CMESSAGE_FILE_METADATA: {
            copy_location = (char*)&wcmsg_tcp->file_metadata;
            break;
        }
        case CMESSAGE_FILE_DATA: {
            copy_location = (char*)&wcmsg_tcp->file;
            break;
        }
        default: {
            // This is unreachable code, only here for consistency's sake
            deallocate_region(wcmsg_tcp);
            LOG_ERROR("Not a valid server wcmsg type");
            return;
        }
    }

    // Build wcmsg
    // Init header to 0 to prevent sending uninitialized packets over the network
    memset(wcmsg_tcp, 0, sizeof(*wcmsg_tcp));
    wcmsg_tcp->type = message_type;
    memcpy(copy_location, payload, type_size + data_size);
    // Send wcmsg
    send_wcmsg(wcmsg_tcp);
    // Free wcmsg
    deallocate_region(wcmsg_tcp);
}

int multithreaded_sync_tcp_packets(void* opaque) {
    /*
        Thread to send and receive all TCP packets (clipboard and file)

        Arguments:
            opaque (void*): any arg to be passed to thread

        Return:
            (int): 0 on success
    */
    SocketContext* socket_context = &packet_tcp_context;

    last_tcp_ping_id = 0;

    WhistTimer last_ack;
    WhistTimer statistics_timer;
    start_timer(&last_ack);
    bool successful_read_or_pull = false;

    while (run_sync_packets_threads) {
        // Ack the connection every 50 ms
        if (get_timer(&last_ack) > 0.05) {
            int ret = ack(socket_context);
            if (ret != 0) {
                LOG_WARNING("Lost TCP Connection (Error: %d)", get_last_network_error());
                send_tcp_reconnect_message();
            }
            start_timer(&last_ack);
        }

        successful_read_or_pull = false;

        // Update TCP ping and reconnect TCP if needed (TODO: does that function do too much?)
        update_tcp_ping();

        TIME_RUN(WhistPacket* packet = read_packet(socket_context, true), NETWORK_READ_PACKET_TCP,
                 statistics_timer);

        if (packet) {
            TIME_RUN(handle_server_message((WhistServerMessage*)packet->data,
                                           (size_t)packet->payload_size),
                     SERVER_HANDLE_MESSAGE_TCP, statistics_timer);
            free_packet(socket_context, packet);
        }

        // PULL CLIPBOARD HANDLER
        ClipboardData* clipboard_chunk = pull_clipboard_chunk();
        if (clipboard_chunk) {
            create_and_send_tcp_wcmsg(CMESSAGE_CLIPBOARD, (char*)clipboard_chunk);
            deallocate_region(clipboard_chunk);

            successful_read_or_pull = true;
        }

        // READ FILE HANDLER
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

                create_and_send_tcp_wcmsg(CMESSAGE_FILE_METADATA, (char*)file_metadata);
                // Free file chunk
                deallocate_region(file_metadata);
            } else {
                // If we successfully read a chunk, then send it to the server.
                create_and_send_tcp_wcmsg(CMESSAGE_FILE_DATA, (char*)file_chunk);
                // Free file chunk
                deallocate_region(file_chunk);
            }

            successful_read_or_pull = true;
        }

        // We want to continue pumping pull_clipboard_chunk and
        //     file_synchronizer_read_next_file_chunk
        //     without sleeping if either of them are currently pulling/reading chunks. Otherwise,
        //     sleep to target one loop every 25 ms.
        if (!successful_read_or_pull && get_timer(&last_ack) * MS_IN_SECOND < 25.0) {
            whist_sleep(max(1, (int)(25.0 - get_timer(&last_ack) * MS_IN_SECOND)));
        }
    }
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

void init_packet_synchronizers(WhistRenderer* whist_renderer) {
    /*
        Initialize the packet synchronizer threads for UDP and TCP.
    */
    if (run_sync_packets_threads == true) {
        LOG_FATAL("Packet synchronizers are already running!");
    }
    run_sync_packets_threads = true;
    sync_udp_packets_thread = whist_create_thread(multithreaded_sync_udp_packets,
                                                  "multithreaded_sync_udp_packets", whist_renderer);
    sync_tcp_packets_thread =
        whist_create_thread(multithreaded_sync_tcp_packets, "multithreaded_sync_tcp_packets", NULL);
}

void destroy_packet_synchronizers() {
    /*
        Destroy and wait on the packet synchronizer threads for UDP and TCP.
    */
    if (run_sync_packets_threads == false) {
        LOG_ERROR("Packet synchronizers have not been initialized!");
        return;
    }
    run_sync_packets_threads = false;
    whist_wait_thread(sync_tcp_packets_thread, NULL);
    whist_wait_thread(sync_udp_packets_thread, NULL);
}
