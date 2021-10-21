/**
 * Copyright Fractal Computers, Inc. 2020
 * @file handle_client_message.c
 * @brief This file contains all the code for server-side processing of messages
 *        received from a client
============================
Usage
============================

handle_client_message() must be called on any received message from a client.
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>

#include <fractal/input/input.h>
#include <fractal/network/network.h>
#include <fractal/utils/clock.h>
#include <fractal/logging/logging.h>
#include <fractal/logging/log_statistic.h>
#include <fractal/logging/error_monitor.h>
#include "client.h"
#include "handle_client_message.h"
#include "network.h"

#ifdef _WIN32
#include <fractal/utils/windows_utils.h>
#endif

extern Client client;

#define VIDEO_BUFFER_SIZE 25
#define MAX_VIDEO_INDEX 500
extern FractalPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

#define AUDIO_BUFFER_SIZE 100
#define MAX_NUM_AUDIO_INDICES 3
extern FractalPacket audio_buffer[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];

extern volatile int max_bitrate;
extern volatile int client_width;
extern volatile int client_height;
extern volatile int client_dpi;
extern volatile CodecType client_codec_type;
extern volatile bool update_device;
extern volatile FractalOSType client_os;

extern volatile bool stop_streaming;
extern volatile bool wants_iframe;
extern volatile bool update_encoder;
extern InputDevice *input_device;

extern int host_id;

/*
============================
Private Functions
============================
*/

static int handle_user_input_message(FractalClientMessage *fcmsg);
static int handle_keyboard_state_message(FractalClientMessage *fcmsg);
static int handle_streaming_toggle_message(FractalClientMessage *fcmsg);
static int handle_bitrate_message(FractalClientMessage *fcmsg);
static int handle_ping_message(FractalClientMessage *fcmsg);
static int handle_tcp_ping_message(FractalClientMessage *fcmsg);
static int handle_dimensions_message(FractalClientMessage *fcmsg);
static int handle_clipboard_message(FractalClientMessage *fcmsg);
static int handle_audio_nack_message(FractalClientMessage *fcmsg);
static int handle_video_nack_message(FractalClientMessage *fcmsg);
static int handle_iframe_request_message(FractalClientMessage *fcmsg);
static int handle_quit_message(FractalClientMessage *fcmsg);
static int handle_init_message(FractalClientMessage *fcmsg);

/*
============================
Public Function Implementations
============================
*/

int handle_client_message(FractalClientMessage *fcmsg) {
    /*
        Handle message from the client.

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */
    static clock temp_clock;

    switch (fcmsg->type) {
        case MESSAGE_KEYBOARD:
        case MESSAGE_MOUSE_BUTTON:
        case MESSAGE_MOUSE_WHEEL:
        case MESSAGE_MOUSE_MOTION:
        case MESSAGE_MULTIGESTURE:
            start_timer(&temp_clock);
            int r = handle_user_input_message(fcmsg);
            log_double_statistic("handle_user_input_message time (ms)",
                                 get_timer(temp_clock) * MS_IN_SECOND);
            return r;
        case MESSAGE_KEYBOARD_STATE:
            return handle_keyboard_state_message(fcmsg);
        case MESSAGE_START_STREAMING:
        case MESSAGE_STOP_STREAMING:
            return handle_streaming_toggle_message(fcmsg);
        case MESSAGE_MBPS:
            return handle_bitrate_message(fcmsg);
        case MESSAGE_PING:
            return handle_ping_message(fcmsg);
        case MESSAGE_TCP_PING:
            return handle_tcp_ping_message(fcmsg);
        case MESSAGE_DIMENSIONS:
            return handle_dimensions_message(fcmsg);
        case CMESSAGE_CLIPBOARD:
            return handle_clipboard_message(fcmsg);
        case MESSAGE_AUDIO_NACK:
        case MESSAGE_AUDIO_BITARRAY_NACK:
            return handle_audio_nack_message(fcmsg);
        case MESSAGE_VIDEO_NACK:
        case MESSAGE_VIDEO_BITARRAY_NACK:
            return handle_video_nack_message(fcmsg);
        case MESSAGE_IFRAME_REQUEST:
            return handle_iframe_request_message(fcmsg);
        case CMESSAGE_QUIT:
            return handle_quit_message(fcmsg);
        case MESSAGE_DISCOVERY_REQUEST:
            return handle_init_message(fcmsg);
        default:
            LOG_WARNING(
                "Unknown FractalClientMessage Received. "
                "(Type: %d)",
                fcmsg->type);
            return -1;
    }
}

/*
============================
Private Function Implementations
============================
*/

static int handle_user_input_message(FractalClientMessage *fcmsg) {
    /*
        Handle a user input message.

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Replay user input (keyboard or mouse presses)
    if (input_device) {
        if (!replay_user_input(input_device, fcmsg)) {
            LOG_WARNING("Failed to replay input!");
#ifdef _WIN32
            // TODO: Ensure that any password can be used here
            init_desktop(input_device, "password1234567.");
#endif
        }
    }

    return 0;
}

// TODO: Unix version missing
static int handle_keyboard_state_message(FractalClientMessage *fcmsg) {
    /*
        Handle a user keyboard state change message. Synchronize client and
        server keyboard state

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    update_keyboard_state(input_device, fcmsg);
    return 0;
}

static int handle_streaming_toggle_message(FractalClientMessage *fcmsg) {
    /*
        Stop encoding and sending frames if the client requests it to save resources

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    if (fcmsg->type == MESSAGE_STOP_STREAMING) {
        LOG_INFO("MSG RECEIVED TO STOP STREAMING");
        stop_streaming = true;
    } else if (fcmsg->type == MESSAGE_START_STREAMING && stop_streaming == true) {
        // Extra check that `stop_streaming == true` is to ignore erroneous extra
        // MESSAGE_START_STREAMING messages
        LOG_INFO("MSG RECEIVED TO START STREAMING AGAIN");
        stop_streaming = false;
        wants_iframe = true;
    } else {
        return -1;
    }
    return 0;
}

static int handle_bitrate_message(FractalClientMessage *fcmsg) {
    /*
        Handle a user bitrate change message and update MBPS.

        NOTE: idk how to handle this

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("MSG RECEIVED FOR MBPS: %f/%f", fcmsg->bitrate_data.bitrate / 1024.0 / 1024.0,
             fcmsg->bitrate_data.burst_bitrate / 1024.0 / 1024.0);
    // Clamp the bitrates, preferring to clamp at MAX
    fcmsg->bitrate_data.bitrate =
        min(max(fcmsg->bitrate_data.bitrate, MINIMUM_BITRATE), MAXIMUM_BITRATE);
    fcmsg->bitrate_data.burst_bitrate =
        min(max(fcmsg->bitrate_data.burst_bitrate, MINIMUM_BURST_BITRATE), MAXIMUM_BURST_BITRATE);
    LOG_INFO("Clamped to %f/%f", fcmsg->bitrate_data.bitrate / 1024.0 / 1024.0,
             fcmsg->bitrate_data.burst_bitrate / 1024.0 / 1024.0);
    // Set the new bitrate data (for the video encoder)
    max_bitrate = fcmsg->bitrate_data.bitrate;

    // Use the burst bitrate to update the client's UDP packet throttle context
    network_throttler_set_burst_bitrate(client.udp_context.network_throttler,
                                        fcmsg->bitrate_data.burst_bitrate);

    // Update the encoder using the new bitrate
    update_encoder = true;
    return 0;
}

static int handle_ping_message(FractalClientMessage *fcmsg) {
    /*
        Handle a client ping (alive) message.

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Ping Received - Ping ID %d", fcmsg->ping_id);

    // Update ping timer
    start_timer(&(client.last_ping));

    // Send pong reply
    FractalServerMessage fsmsg_response = {0};
    fsmsg_response.type = MESSAGE_PONG;
    fsmsg_response.ping_id = fcmsg->ping_id;
    int ret = 0;

    if (send_udp_packet_from_payload(&(client.udp_context), PACKET_MESSAGE,
                                     (uint8_t *)&fsmsg_response, sizeof(fsmsg_response), 1) < 0) {
        LOG_WARNING("Could not send Ping");
        ret = -1;
    }

    return ret;
}

static int handle_tcp_ping_message(FractalClientMessage *fcmsg) {
    /*
        Handle a client TCP ping message.

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("TCP Ping Received - TCP Ping ID %d", fcmsg->ping_id);

    // Update ping timer
    start_timer(&(client.last_ping));

    // Send pong reply
    FractalServerMessage fsmsg_response = {0};
    fsmsg_response.type = MESSAGE_TCP_PONG;
    fsmsg_response.ping_id = fcmsg->ping_id;
    int ret = 0;

    if (send_tcp_packet_from_payload(&(client.tcp_context), PACKET_MESSAGE,
                                     (uint8_t *)&fsmsg_response, sizeof(fsmsg_response)) < 0) {
        LOG_WARNING("Could not send TCP Ping to client");
        ret = -1;
    }

    return ret;
}

static int handle_dimensions_message(FractalClientMessage *fcmsg) {
    /*
        Handle a user dimensions change message.

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Update knowledge of client monitor dimensions
    LOG_INFO("Request to use codec %d / dimensions %dx%d / dpi %d received",
             fcmsg->dimensions.codec_type, fcmsg->dimensions.width, fcmsg->dimensions.height,
             fcmsg->dimensions.dpi);
    if (client_width != fcmsg->dimensions.width || client_height != fcmsg->dimensions.height ||
        client_codec_type != fcmsg->dimensions.codec_type || client_dpi != fcmsg->dimensions.dpi) {
        client_width = fcmsg->dimensions.width;
        client_height = fcmsg->dimensions.height;
        client_codec_type = fcmsg->dimensions.codec_type;
        client_dpi = fcmsg->dimensions.dpi;
        // Update device if knowledge changed
        update_device = true;
    } else {
        LOG_INFO(
            "No need to update the decoder as the requested parameters are the same as the "
            "currently chosen parameters");
    }
    return 0;
}

static int handle_clipboard_message(FractalClientMessage *fcmsg) {
    /*
        Handle a clipboard copy message.

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Update clipboard with message
    LOG_INFO("Received Clipboard Data! %d", fcmsg->clipboard.type);
    if (!clipboard_synchronizer_set_clipboard_chunk(&fcmsg->clipboard)) {
        LOG_ERROR("Failed to set local clipboard from client message.");
        return -1;
    }
    return 0;
}

static void handle_nack_single_audio_packet(int packet_id, int packet_index) {
    // LOG_INFO("Audio NACK requested for: ID %d Index %d",
    // packet_id, packet_index);
    FractalPacket *audio_packet = &audio_buffer[packet_id % AUDIO_BUFFER_SIZE][packet_index];
    int len = get_packet_size(audio_packet);
    if (audio_packet->id == packet_id) {
        LOG_INFO(
            "NACKed audio packet %d found of length %d. "
            "Relaying!",
            packet_id, len);
        audio_packet->is_a_nack = true;
        send_udp_packet(&(client.udp_context), audio_packet, len);
    }
    // If we were asked for an invalid index, just ignore it
    else if (packet_index < audio_packet->num_indices) {
        LOG_WARNING(
            "NACKed audio packet %d %d not found, ID %d %d was "
            "located instead.",
            packet_id, packet_index, audio_packet->id, audio_packet->index);
    }
}

static int handle_audio_nack_message(FractalClientMessage *fcmsg) {
    /*
        Handle a audio nack message and relay the packet

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    if (fcmsg->type == MESSAGE_AUDIO_NACK) {
        handle_nack_single_audio_packet(fcmsg->simple_nack.id, fcmsg->simple_nack.index);
    } else {
        // fcmsg->type == MESSAGE_AUDIO_BITARRAY_NACK
        BitArray *bit_arr = bit_array_create(fcmsg->bitarray_audio_nack.numBits);
        bit_array_clear_all(bit_arr);

        memcpy(bit_array_get_bits(bit_arr), fcmsg->bitarray_audio_nack.ba_raw,
               BITS_TO_CHARS(fcmsg->bitarray_audio_nack.numBits));

        for (int i = fcmsg->bitarray_audio_nack.index; i < fcmsg->bitarray_audio_nack.numBits;
             i++) {
            if (bit_array_test_bit(bit_arr, i)) {
                handle_nack_single_audio_packet(fcmsg->bitarray_audio_nack.id, i);
            }
        }
        bit_array_free(bit_arr);
    }
    return 0;
}

static void handle_nack_single_video_packet(int packet_id, int packet_index) {
    // LOG_INFO("Video NACK requested for: ID %d Index %d",
    // fcmsg->nack_data.simple_nack.id, fcmsg->nack_data.simple_nack.index);
    FractalPacket *video_packet = &video_buffer[packet_id % VIDEO_BUFFER_SIZE][packet_index];
    int len = get_packet_size(video_packet);
    if (video_packet->id == packet_id) {
        LOG_INFO(
            "NACKed video packet ID %d Index %d found of "
            "length %d. Relaying!",
            packet_id, packet_index, len);
        send_udp_packet(&(client.udp_context), video_packet, len);
    }

    // If we were asked for an invalid index, just ignore it
    else if (packet_index < video_packet->num_indices) {
        LOG_WARNING(
            "NACKed video packet %d %d not found, ID %d %d was "
            "located instead.",
            packet_id, packet_index, video_packet->id, video_packet->index);
    }
}

static int handle_video_nack_message(FractalClientMessage *fcmsg) {
    /*
        Handle a video nack message and relay the packet

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    if (fcmsg->type == MESSAGE_VIDEO_NACK) {
        handle_nack_single_video_packet(fcmsg->simple_nack.id, fcmsg->simple_nack.index);
    } else {
        // fcmsg->type == MESSAGE_VIDEO_BITARRAY_NACK
        BitArray *bit_arr = bit_array_create(fcmsg->bitarray_video_nack.numBits);
        bit_array_clear_all(bit_arr);

        memcpy(bit_array_get_bits(bit_arr), fcmsg->bitarray_video_nack.ba_raw,
               BITS_TO_CHARS(fcmsg->bitarray_video_nack.numBits));

        for (int i = fcmsg->bitarray_video_nack.index; i < fcmsg->bitarray_video_nack.numBits;
             i++) {
            if (bit_array_test_bit(bit_arr, i)) {
                handle_nack_single_video_packet(fcmsg->bitarray_video_nack.id, i);
            }
        }
        bit_array_free(bit_arr);
    }

    return 0;
}

static int handle_iframe_request_message(FractalClientMessage *fcmsg) {
    /*
        Handle an IFrame request message

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Request for i-frame found: Creating iframe");
    if (fcmsg->reinitialize_encoder) {
        // Wants to completely reinitialize the encoder
        update_encoder = true;
        wants_iframe = true;
    } else {
        // Wants only an iframe
        wants_iframe = true;
    }
    return 0;
}

static int handle_quit_message(FractalClientMessage *fcmsg) {
    /*
        Handle a user quit message

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    UNUSED(fcmsg);
    if (start_quitting_client() != 0) {
        LOG_ERROR("Failed to start quitting client.");
        return -1;
    }
    LOG_INFO("Client successfully started quitting.");
    return 0;
}

static int handle_init_message(FractalClientMessage *cfcmsg) {
    /*
        Handle a user init message

        Arguments:
            fcmsg (FractalClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Receiving a message time packet");

    FractalDiscoveryRequestMessage fcmsg = cfcmsg->discoveryRequest;

    // Handle time
    set_time_data(&fcmsg.time_data);
    client_os = fcmsg.os;

    error_monitor_set_username(fcmsg.user_email);

    return 0;
}
