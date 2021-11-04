/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
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
#include <fractal/core/fractal.h>
#include <fractal/utils/clock.h>
#include <fractal/network/network.h>
#include <fractal/logging/log_statistic.h>
#include <fractal/logging/logging.h>
#include "handle_server_message.h"
#include "network.h"
#include "audio.h"
#include "video.h"
#include "sync_packets.h"
#include "client_utils.h"

// Updater variables
extern SocketContext packet_udp_context;
extern SocketContext packet_tcp_context;
bool connected = true;
// Ping variables
clock last_ping_timer;
volatile int last_ping_id;
volatile int last_pong_id;
volatile int ping_failures;
// TCP ping variables
clock last_tcp_ping_timer;
volatile int last_tcp_ping_id;
volatile int last_tcp_pong_id;
extern volatile double latency;
// MBPS variables
extern volatile int client_max_bitrate;
extern volatile int max_burst_bitrate;
extern volatile bool update_bitrate;
// dimension variables
extern volatile int server_width;
extern volatile int server_height;
extern volatile CodecType server_codec_type;
extern volatile CodecType output_codec_type;
extern volatile int output_width;
extern volatile int output_height;

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

    static clock last_new_ping_timer;
    static bool timer_initialized = false;
    if (!timer_initialized) {
        start_timer(&last_new_ping_timer);
        timer_initialized = true;
    }

    // If it's been 1 second since the last ping, we should warn
    if (get_timer(last_ping_timer) > 1.0) {
        LOG_WARNING("No ping sent or pong received in over a second");
    }

    // If we're waiting for a ping, and it's been 600ms, then that ping will be
    // noted as failed
    if (last_ping_id != last_pong_id && get_timer(last_new_ping_timer) > 0.6) {
        LOG_WARNING("Ping received no response: %d", last_ping_id);
        // Keep track of failures, and exit if too many failures
        last_pong_id = last_ping_id;
        ++ping_failures;
        if (ping_failures == 3) {
            // we make this a LOG_WARNING so it doesn't clog up Sentry, as this
            // error happens periodically but we have recovery systems in place
            // for streaming interruption/connection loss
            LOG_WARNING("Server disconnected: 3 consecutive ping failures.");
            connected = false;
        }
    }

    // if we've received the last ping, send another
    if (last_ping_id == last_pong_id && get_timer(last_ping_timer) > 0.5) {
        send_ping(last_ping_id + 1);
        start_timer(&last_new_ping_timer);
    }

    // if we haven't received the last ping, send the same ping
    if (last_ping_id != last_pong_id && get_timer(last_ping_timer) > 0.21) {
        send_ping(last_ping_id);
    }
}

void update_tcp_ping() {
    /*
       If no valid TCP pong has been received or sending a TCP ping is failing, then
       send a TCP reconnection request to the server. This is agnostic of whether
       the lost connection was caused by the client or the server.
    */

    static clock last_new_ping_timer;
    static bool timer_initialized = false;
    if (!timer_initialized) {
        start_timer(&last_new_ping_timer);
        timer_initialized = true;
    }

    // If it's been 4 seconds since the last ping, we should warn
    if (get_timer(last_tcp_ping_timer) > 4.0) {
        LOG_WARNING("No TCP ping sent or pong received in over a second");
    }

    // If we're waiting for a ping, and it's been 1s, then that ping will be
    // noted as failed
    if (last_tcp_ping_id != last_tcp_pong_id && get_timer(last_new_ping_timer) > 1.0) {
        LOG_WARNING("TCP ping received no response: %d", last_tcp_ping_id);

        // Only if we successfully recover the TCP connection should we continue
        //     as if the ping was successful.
        if (send_tcp_reconnect_message() == 0) {
            last_tcp_pong_id = last_tcp_ping_id;
        }
    }

    // if we've received the last ping, send another
    if (last_tcp_ping_id == last_tcp_pong_id && get_timer(last_tcp_ping_timer) > 2.0) {
        send_tcp_ping(last_tcp_ping_id + 1);
        start_timer(&last_new_ping_timer);
    }
}

void update_server_bitrate() {
    /*
        Tell the server to update the bitrate of its video if needed.
    */
    if (update_bitrate) {
        update_bitrate = false;
        FractalClientMessage fcmsg = {0};
        fcmsg.type = MESSAGE_MBPS;
        fcmsg.bitrate_data.bitrate = client_max_bitrate;
        fcmsg.bitrate_data.burst_bitrate = max_burst_bitrate;
        fcmsg.bitrate_data.fec_packet_ratio = FEC_PACKET_RATIO;
        LOG_INFO("Asking for server MBPS to be %f/%f/%f",
                 fcmsg.bitrate_data.bitrate / 1024.0 / 1024.0,
                 fcmsg.bitrate_data.burst_bitrate / 1024.0 / 1024.0,
                 fcmsg.bitrate_data.fec_packet_ratio);
        send_fcmsg(&fcmsg);
    }
}

#define TIME_RUN(line, name, timer) \
    start_timer(&timer);            \
    line;                           \
    log_double_statistic(name " time (ms)", get_timer(timer) * MS_IN_SECOND);

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
    bool* run_sync_packets = (bool*)opaque;
    SocketContext* socket_context = &packet_udp_context;

    // we initialize latency here because on macOS, latency would not initialize properly in
    // its global declaration. We start at 25ms before the first ping.
    latency = 25.0 / 1000.0;
    last_ping_id = 0;
    ping_failures = 0;

    clock last_ack;
    clock statistics_timer;
    start_timer(&last_ack);

    // Initialize dimensions prior to update_video and receive_video calls
    if (server_width != output_width || server_height != output_height ||
        server_codec_type != output_codec_type) {
        send_message_dimensions();
    }

    while (*run_sync_packets) {
        // Ack the connection every 5 seconds
        if (get_timer(last_ack) > 5.0) {
            ack(socket_context);
            start_timer(&last_ack);
        }

        update_server_bitrate();
        update_ping();
        TIME_RUN(update_video(), "update_video", statistics_timer);
        TIME_RUN(update_audio(), "update_audio", statistics_timer);
        TIME_RUN(FractalPacket* packet = read_packet(socket_context, true), "read_packet (udp)",
                 statistics_timer);

        if (!packet) {
            continue;
        }

        switch (packet->type) {
            case PACKET_VIDEO: {
                TIME_RUN(receive_video(packet), "receive_video", statistics_timer);
                break;
            }
            case PACKET_AUDIO: {
                TIME_RUN(receive_audio(packet), "receive_audio", statistics_timer);
                break;
            }
            case PACKET_MESSAGE: {
                TIME_RUN(handle_server_message((FractalServerMessage*)packet->data,
                                               (size_t)packet->payload_size),
                         "handle_server_message (udp)", statistics_timer);
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

int multithreaded_sync_tcp_packets(void* opaque) {
    /*
        Thread to send and receive all TCP packets (clipboard and file)

        Arguments:
            opaque (void*): any arg to be passed to thread

        Return:
            (int): 0 on success
    */

    bool* run_sync_packets = (bool*)opaque;
    SocketContext* socket_context = &packet_tcp_context;

    last_tcp_ping_id = 0;

    clock last_ack;
    clock statistics_timer;
    start_timer(&last_ack);

    while (*run_sync_packets) {
        // Ack the connection every 50 ms
        if (get_timer(last_ack) > 0.05) {
            int ret = ack(socket_context);
            if (ret != 0) {
                LOG_WARNING("Lost TCP Connection (Error: %d)", get_last_network_error());
                send_tcp_reconnect_message();
            }
            start_timer(&last_ack);
        }

        // Update TCP ping and reconnect TCP if needed (TODO: does that function do too much?)
        update_tcp_ping();

        TIME_RUN(FractalPacket* packet = read_packet(socket_context, true), "read_packet (tcp)",
                 statistics_timer);

        if (packet) {
            TIME_RUN(handle_server_message((FractalServerMessage*)packet->data,
                                           (size_t)packet->payload_size),
                     "handle_server_message (tcp)", statistics_timer);
            free_packet(socket_context, packet);
        }

        ClipboardData* clipboard_chunk = clipboard_synchronizer_get_next_clipboard_chunk();
        if (clipboard_chunk) {
            FractalClientMessage* fcmsg = allocate_region(
                sizeof(FractalClientMessage) + sizeof(ClipboardData) + clipboard_chunk->size);

            // Init header to 0 to prevent sending uninitialized packets over the network
            memset(fcmsg, 0, sizeof(FractalClientMessage));
            fcmsg->type = CMESSAGE_CLIPBOARD;
            memcpy(&fcmsg->clipboard, clipboard_chunk,
                   sizeof(ClipboardData) + clipboard_chunk->size);
            send_fcmsg(fcmsg);
            deallocate_region(fcmsg);
            deallocate_region(clipboard_chunk);
        }

        if (is_clipboard_synchronizing()) {
            // We want to continue pumping read_packet or get_next_clipboard_chunk
            // until we're done synchronizing.
            continue;
        }

        // Sleep to target one loop every 25 ms.
        if (get_timer(last_ack) * MS_IN_SECOND < 25.0) {
            fractal_sleep(max(1, (int)(25.0 - get_timer(last_ack) * MS_IN_SECOND)));
        }
    }
    return 0;
}
