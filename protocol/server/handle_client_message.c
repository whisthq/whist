/**
 * Copyright 2021 Whist Technologies, Inc.
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

#include <whist/core/whist.h>

#include <whist/input/input.h>
#include <whist/network/network.h>
#include <whist/utils/clock.h>
#include <whist/logging/logging.h>
#include <whist/logging/log_statistic.h>
#include <whist/logging/error_monitor.h>
#include "main.h"
#include "client.h"
#include "handle_client_message.h"
#include "network.h"
#include "server_statistic.h"

/*
============================
Private Functions
============================
*/

static int handle_user_input_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_keyboard_state_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_streaming_toggle_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_bitrate_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_ping_message(Client *client, WhistClientMessage *fcmsg);
static int handle_tcp_ping_message(Client *client, WhistClientMessage *fcmsg);
static int handle_dimensions_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_clipboard_message(WhistClientMessage *fcmsg);
static int handle_nack_message(Client *client, WhistClientMessage *fcmsg);
static int handle_iframe_request_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_quit_message(whist_server_state *state, WhistClientMessage *fcmsg);
static int handle_init_message(whist_server_state *state, WhistClientMessage *fcmsg);

/*
============================
Public Function Implementations
============================
*/

int handle_client_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle message from the client.

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

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
            int r = handle_user_input_message(state, fcmsg);
            log_double_statistic(CLIENT_HANDLE_USERINPUT_TIME,
                                 get_timer(temp_clock) * MS_IN_SECOND);
            return r;
        case MESSAGE_KEYBOARD_STATE:
            return handle_keyboard_state_message(state, fcmsg);
        case MESSAGE_START_STREAMING:
        case MESSAGE_STOP_STREAMING:
            return handle_streaming_toggle_message(state, fcmsg);
        case MESSAGE_MBPS:
            return handle_bitrate_message(state, fcmsg);
        case MESSAGE_PING:
            return handle_ping_message(&state->client, fcmsg);
        case MESSAGE_TCP_PING:
            return handle_tcp_ping_message(&state->client, fcmsg);
        case MESSAGE_DIMENSIONS:
            return handle_dimensions_message(state, fcmsg);
        case CMESSAGE_CLIPBOARD:
            return handle_clipboard_message(fcmsg);
        case MESSAGE_NACK:
        case MESSAGE_BITARRAY_NACK:
            return handle_nack_message(&state->client, fcmsg);
        case MESSAGE_IFRAME_REQUEST:
            return handle_iframe_request_message(state, fcmsg);
        case CMESSAGE_QUIT:
            return handle_quit_message(state, fcmsg);
        case MESSAGE_DISCOVERY_REQUEST:
            return handle_init_message(state, fcmsg);
        default:
            LOG_ERROR(
                "Unknown WhistClientMessage Received. "
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

static int handle_user_input_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a user input message.

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Replay user input (keyboard or mouse presses)
    if (state->input_device) {
        if (!replay_user_input(state->input_device, fcmsg)) {
            LOG_WARNING("Failed to replay input!");
            return -1;
        }
    }

    return 0;
}

// TODO: Unix version missing
static int handle_keyboard_state_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a user keyboard state change message. Synchronize client and
        server keyboard state

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    update_keyboard_state(state->input_device, fcmsg);
    return 0;
}

static int handle_streaming_toggle_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Stop encoding and sending frames if the client requests it to save resources

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    if (fcmsg->type == MESSAGE_STOP_STREAMING) {
        LOG_INFO("Received message to stop streaming");
        state->stop_streaming = true;
    } else if (fcmsg->type == MESSAGE_START_STREAMING && state->stop_streaming == true) {
        // Extra check that `stop_streaming == true` is to ignore erroneous extra
        // MESSAGE_START_STREAMING messages
        LOG_INFO("Received message to start streaming again.");
        state->stop_streaming = false;
        state->wants_iframe = true;
    } else {
        LOG_WARNING("Received streaming message to %s streaming, but we're already in that state!",
                    state->stop_streaming ? "stop" : "start");
        return -1;
    }
    return 0;
}

static int handle_bitrate_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a user bitrate change message and update MBPS.

        NOTE: idk how to handle this

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("MSG RECEIVED FOR MBPS: %f/%f/%f", fcmsg->bitrate_data.bitrate / 1024.0 / 1024.0,
             fcmsg->bitrate_data.burst_bitrate / 1024.0 / 1024.0,
             fcmsg->bitrate_data.fec_packet_ratio);
    // Clamp the bitrates, preferring to clamp at MAX
    fcmsg->bitrate_data.bitrate =
        min(max(fcmsg->bitrate_data.bitrate, MINIMUM_BITRATE), MAXIMUM_BITRATE);
    fcmsg->bitrate_data.burst_bitrate =
        min(max(fcmsg->bitrate_data.burst_bitrate, MINIMUM_BURST_BITRATE), MAXIMUM_BURST_BITRATE);
    LOG_INFO("Clamped to %f/%f", fcmsg->bitrate_data.bitrate / 1024.0 / 1024.0,
             fcmsg->bitrate_data.burst_bitrate / 1024.0 / 1024.0);
    // Set the new bitrate data (for the video encoder)
    state->max_bitrate = fcmsg->bitrate_data.bitrate;

    // Update the UDP Context's burst bitrate and fec ratio
    udp_update_bitrate_settings(&state->client.udp_context, fcmsg->bitrate_data.burst_bitrate,
                                fcmsg->bitrate_data.fec_packet_ratio);

    // Update the encoder using the new bitrate
    state->update_encoder = true;
    return 0;
}

static int handle_ping_message(Client *client, WhistClientMessage *fcmsg) {
    /*
        Handle a client ping (alive) message.

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Ping Received - Ping ID %d", fcmsg->ping_id);

    // Update ping timer
    start_timer(&client->last_ping);

    // Send pong reply
    WhistServerMessage fsmsg_response = {0};
    fsmsg_response.type = MESSAGE_PONG;
    fsmsg_response.ping_id = fcmsg->ping_id;

    if (send_packet(&client->udp_context, PACKET_MESSAGE, (uint8_t *)&fsmsg_response,
                    sizeof(fsmsg_response), 1) < 0) {
        LOG_WARNING("Failed to send UDP pong");
        return -1;
    }

    return 0;
}

static int handle_tcp_ping_message(Client *client, WhistClientMessage *fcmsg) {
    /*
        Handle a client TCP ping message.

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("TCP Ping Received - TCP Ping ID %d", fcmsg->ping_id);

    // Update ping timer
    start_timer(&client->last_ping);

    // Send pong reply
    WhistServerMessage fsmsg_response = {0};
    fsmsg_response.type = MESSAGE_TCP_PONG;
    fsmsg_response.ping_id = fcmsg->ping_id;

    if (send_packet(&client->tcp_context, PACKET_MESSAGE, (uint8_t *)&fsmsg_response,
                    sizeof(fsmsg_response), -1) < 0) {
        LOG_WARNING("Failed to send TCP pong");
        return -1;
    }

    return 0;
}

static int handle_dimensions_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a user dimensions change message.

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Update knowledge of client monitor dimensions
    LOG_INFO("Request to use codec %d / dimensions %dx%d / dpi %d received",
             fcmsg->dimensions.codec_type, fcmsg->dimensions.width, fcmsg->dimensions.height,
             fcmsg->dimensions.dpi);
    if (state->client_width != fcmsg->dimensions.width ||
        state->client_height != fcmsg->dimensions.height ||
        state->client_codec_type != fcmsg->dimensions.codec_type ||
        state->client_dpi != fcmsg->dimensions.dpi) {
        state->client_width = fcmsg->dimensions.width;
        state->client_height = fcmsg->dimensions.height;
        state->client_codec_type = fcmsg->dimensions.codec_type;
        state->client_dpi = fcmsg->dimensions.dpi;
        // Update device if knowledge changed
        state->update_device = true;
    } else {
        LOG_INFO(
            "No need to update the decoder as the requested parameters are the same as the "
            "currently chosen parameters");
    }
    return 0;
}

static int handle_clipboard_message(WhistClientMessage *fcmsg) {
    /*
        Handle a clipboard copy message.

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Update clipboard with message
    LOG_INFO("Received Clipboard Data! %d", fcmsg->clipboard.type);
    push_clipboard_chunk(&fcmsg->clipboard);
    return 0;
}

static int handle_nack_message(Client *client, WhistClientMessage *fcmsg) {
    /*
        Handle a video nack message and relay the packet

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    if (fcmsg->type == MESSAGE_NACK) {
        udp_nack(&client->udp_context, fcmsg->simple_nack.type, fcmsg->simple_nack.id,
                 fcmsg->simple_nack.index);
    } else {
        // fcmsg->type == MESSAGE_VIDEO_BITARRAY_NACK
        BitArray *bit_arr = bit_array_create(fcmsg->bitarray_nack.numBits);
        bit_array_clear_all(bit_arr);

        memcpy(bit_array_get_bits(bit_arr), fcmsg->bitarray_nack.ba_raw,
               BITS_TO_CHARS(fcmsg->bitarray_nack.numBits));

        for (int i = fcmsg->bitarray_nack.index; i < fcmsg->bitarray_nack.numBits; i++) {
            if (bit_array_test_bit(bit_arr, i)) {
                udp_nack(&client->udp_context, fcmsg->bitarray_nack.type, fcmsg->bitarray_nack.id,
                         i);
            }
        }
        bit_array_free(bit_arr);
    }

    return 0;
}

static int handle_iframe_request_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle an IFrame request message

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Request for i-frame found: Creating iframe");
    // Mark as wanting an iframe
    state->wants_iframe = true;
    return 0;
}

static int handle_quit_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a user quit message

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    UNUSED(fcmsg);
    if (start_quitting_client(&state->client) != 0) {
        LOG_ERROR("Failed to start quitting client.");
        return -1;
    }
    LOG_INFO("Client successfully started quitting.");
    return 0;
}

static int handle_init_message(whist_server_state *state, WhistClientMessage *cfcmsg) {
    /*
        Handle a user init message

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Receiving a message time packet");

    WhistDiscoveryRequestMessage fcmsg = cfcmsg->discoveryRequest;

    state->client_os = fcmsg.os;

    error_monitor_set_username(fcmsg.user_email);

    return 0;
}
