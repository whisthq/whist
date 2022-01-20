/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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
#include "state.h"
#include "client.h"
#include "handle_client_message.h"
#include "network.h"
#include "server_statistic.h"

/*
============================
Private Functions
============================
*/

static int handle_user_input_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_keyboard_state_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_streaming_toggle_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_network_settings_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_ping_message(Client *client, WhistClientMessage *wcmsg);
static int handle_tcp_ping_message(Client *client, WhistClientMessage *wcmsg);
static int handle_dimensions_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_clipboard_message(WhistClientMessage *wcmsg);
static int handle_nack_message(Client *client, WhistClientMessage *wcmsg);
static int handle_stream_reset_request_message(whist_server_state *state,
                                               WhistClientMessage *wcmsg);
static int handle_quit_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_init_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_file_metadata_message(WhistClientMessage *wcmsg);
static int handle_file_chunk_message(WhistClientMessage *wcmsg);
static int handle_open_url_message(whist_server_state *state, WhistClientMessage *wcmsg);

/*
============================
Public Function Implementations
============================
*/

int handle_client_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle message from the client.

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */
    static WhistTimer temp_clock;

    switch (wcmsg->type) {
        case MESSAGE_KEYBOARD:
        case MESSAGE_MOUSE_BUTTON:
        case MESSAGE_MOUSE_WHEEL:
        case MESSAGE_MOUSE_MOTION:
        case MESSAGE_MULTIGESTURE:
            start_timer(&temp_clock);
            int r = handle_user_input_message(state, wcmsg);
            log_double_statistic(CLIENT_HANDLE_USERINPUT_TIME,
                                 get_timer(&temp_clock) * MS_IN_SECOND);
            return r;
        case MESSAGE_KEYBOARD_STATE:
            return handle_keyboard_state_message(state, wcmsg);
        case MESSAGE_START_STREAMING:
        case MESSAGE_STOP_STREAMING:
            return handle_streaming_toggle_message(state, wcmsg);
        case MESSAGE_NETWORK_SETTINGS:
            return handle_network_settings_message(state, wcmsg);
        case MESSAGE_UDP_PING:
            return handle_ping_message(&state->client, wcmsg);
        case MESSAGE_TCP_PING:
            return handle_tcp_ping_message(&state->client, wcmsg);
        case MESSAGE_DIMENSIONS:
            return handle_dimensions_message(state, wcmsg);
        case CMESSAGE_CLIPBOARD:
            return handle_clipboard_message(wcmsg);
        case MESSAGE_NACK:
        case MESSAGE_BITARRAY_NACK:
            return handle_nack_message(&state->client, wcmsg);
        case MESSAGE_STREAM_RESET_REQUEST:
            return handle_stream_reset_request_message(state, wcmsg);
        case CMESSAGE_FILE_METADATA:
            return handle_file_metadata_message(wcmsg);
        case CMESSAGE_FILE_DATA:
            return handle_file_chunk_message(wcmsg);
        case CMESSAGE_QUIT:
            return handle_quit_message(state, wcmsg);
        case MESSAGE_DISCOVERY_REQUEST:
            return handle_init_message(state, wcmsg);
        case MESSAGE_OPEN_URL:
            return handle_open_url_message(state, wcmsg);
        default:
            LOG_ERROR(
                "Failed to handle message from client: Unknown WhistClientMessage Received "
                "(Type: %d).",
                wcmsg->type);
            return -1;
    }
}

/*
============================
Private Function Implementations
============================
*/

static int handle_user_input_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user input message.

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Replay user input (keyboard or mouse presses)
    if (state->input_device) {
        if (!replay_user_input(state->input_device, wcmsg)) {
            LOG_WARNING("Failed to replay input!");
            return -1;
        }
    }

    return 0;
}

// TODO: Unix version missing
static int handle_keyboard_state_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user keyboard state change message. Synchronize client and
        server keyboard state

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    update_keyboard_state(state->input_device, wcmsg);
    return 0;
}

static int handle_streaming_toggle_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Stop encoding and sending frames if the client requests it to save resources

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    if (wcmsg->type == MESSAGE_STOP_STREAMING) {
        LOG_INFO("Received message to stop streaming");
        state->stop_streaming = true;
    } else if (wcmsg->type == MESSAGE_START_STREAMING && state->stop_streaming == true) {
        // Extra check that `stop_streaming == true` is to ignore erroneous extra
        // MESSAGE_START_STREAMING messages
        LOG_INFO("Received message to start streaming again.");
        state->stop_streaming = false;
        state->wants_iframe = true;
        state->last_failed_id = -1;
        state->last_iframe_id = -1;
    } else {
        LOG_WARNING("Received streaming message to %s streaming, but we're already in that state!",
                    state->stop_streaming ? "stop" : "start");
    }
    return 0;
}

static int handle_network_settings_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user bitrate change message and update MBPS.

        NOTE: idk how to handle this

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // TODO: udp.c will handle this function internally at some point,
    // So that it is not a WhistClientMessage anymore

    int requested_avg_bitrate = wcmsg->network_settings.bitrate;
    int requested_burst_bitrate = wcmsg->network_settings.burst_bitrate;
    double requested_audio_fec_ratio = wcmsg->network_settings.audio_fec_ratio;
    double requested_video_fec_ratio = wcmsg->network_settings.video_fec_ratio;

    LOG_INFO("Network Settings Message: %fmbps avg/%fmbps burst/%f%% audio FEC/%f%% video FEC",
             requested_avg_bitrate / 1024.0 / 1024.0, requested_burst_bitrate / 1024.0 / 1024.0,
             requested_audio_fec_ratio * 100.0, requested_video_fec_ratio * 100.0);

    // Clamp the bitrates & fec ratio, preferring to clamp at MAX
    int avg_bitrate = min(max(requested_avg_bitrate, MINIMUM_BITRATE), MAXIMUM_BITRATE);
    int burst_bitrate =
        min(max(requested_burst_bitrate, MINIMUM_BURST_BITRATE), MAXIMUM_BURST_BITRATE);
    double audio_fec_ratio = min(max(requested_audio_fec_ratio, 0.0), MAX_FEC_RATIO);
    double video_fec_ratio = min(max(requested_video_fec_ratio, 0.0), MAX_FEC_RATIO);
    // Log an error if clamping was necessary
    if (avg_bitrate != requested_avg_bitrate || burst_bitrate != requested_burst_bitrate ||
        audio_fec_ratio != requested_audio_fec_ratio ||
        video_fec_ratio != requested_video_fec_ratio) {
        LOG_ERROR(
            "Network Settings msg FORCEFULLY CLAMPED: %fmbps avg/%fmbps burst/%f%% audio FEC/%f%% "
            "video FEC",
            avg_bitrate / 1024.0 / 1024.0, burst_bitrate / 1024.0 / 1024.0, audio_fec_ratio * 100.0,
            video_fec_ratio * 100.0);
        wcmsg->network_settings.bitrate = avg_bitrate;
        wcmsg->network_settings.burst_bitrate = burst_bitrate;
        wcmsg->network_settings.audio_fec_ratio = audio_fec_ratio;
        wcmsg->network_settings.video_fec_ratio = video_fec_ratio;
    }

    // Update the UDP Context's burst bitrate and fec ratio
    // TODO: Handle audio_fec_ratio correctly
    udp_update_network_settings(&state->client.udp_context, wcmsg->network_settings);

    // Set the new video encoding parameters,
    // using only the bandwidth that isn't already meant for audio,
    // Or reserved for the audio's FEC packets
    state->requested_video_bitrate = (avg_bitrate - AUDIO_BITRATE) * (1.0 - video_fec_ratio);
    FATAL_ASSERT(state->requested_video_bitrate > 0);
    state->requested_video_codec = wcmsg->network_settings.desired_codec;
    state->requested_video_fps = wcmsg->network_settings.fps;
    // TODO: Implement custom FPS properly
    if (state->requested_video_fps != MAX_FPS) {
        LOG_ERROR("Custom FPS of %d is not possible! %d will be used instead.",
                  state->requested_video_fps, MAX_FPS);
    }
    // Mark the encoder for update using the new video encoding parameters
    state->update_encoder = true;

    return 0;
}

static int handle_tcp_ping_message(Client *client, WhistClientMessage *wcmsg) {
    /*
        Handle a client TCP ping message.

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("TCP Ping Received - TCP Ping ID %d", wcmsg->ping_data.id);

    // Update ping timer
    start_timer(&client->last_ping);

    // Send pong reply
    WhistServerMessage fsmsg_response = {0};
    fsmsg_response.type = MESSAGE_TCP_PONG;
    fsmsg_response.ping_id = wcmsg->ping_data.id;

    if (send_packet(&client->tcp_context, PACKET_MESSAGE, (uint8_t *)&fsmsg_response,
                    sizeof(fsmsg_response), -1) < 0) {
        LOG_WARNING("Failed to send TCP pong");
        return -1;
    }

    return 0;
}

static int handle_dimensions_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user dimensions change message.

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Update knowledge of client monitor dimensions
    LOG_INFO("Request to use dimensions %dx%d / dpi %d received", wcmsg->dimensions.width,
             wcmsg->dimensions.height, wcmsg->dimensions.dpi);
    if (state->client_width != wcmsg->dimensions.width ||
        state->client_height != wcmsg->dimensions.height ||
        state->client_dpi != wcmsg->dimensions.dpi) {
        state->client_width = wcmsg->dimensions.width;
        state->client_height = wcmsg->dimensions.height;
        state->client_dpi = wcmsg->dimensions.dpi;
        // Update device if knowledge changed
        state->update_device = true;
    } else {
        LOG_INFO(
            "No need to update the decoder as the requested parameters are the same as the "
            "currently chosen parameters");
    }
    return 0;
}

static int handle_clipboard_message(WhistClientMessage *wcmsg) {
    /*
        Handle a clipboard copy message.

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Update clipboard with message
    LOG_INFO("Received Clipboard Data! %d", wcmsg->clipboard.type);
    push_clipboard_chunk(&wcmsg->clipboard);
    return 0;
}

static int handle_file_metadata_message(WhistClientMessage *wcmsg) {
    /*
        Handle a file metadata message.

        Arguments:
            wcmsg (WhistClientMessage*): message packet from client
        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    file_synchronizer_open_file_for_writing(&wcmsg->file_metadata);

    return 0;
}

static int handle_file_chunk_message(WhistClientMessage *wcmsg) {
    /*
        Handle a file chunk message.

        Arguments:
            wcmsg (WhistClientMessage*): message packet from client
        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    file_synchronizer_write_file_chunk(&wcmsg->file);

    return 0;
}

static int handle_nack_message(Client *client, WhistClientMessage *wcmsg) {
    /*
        Handle a video nack message and relay the packet

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // TODO: udp.c will handle this function internally at some point,
    // So that it is not a WhistClientMessage anymore

    if (wcmsg->type == MESSAGE_NACK) {
        udp_nack(&client->udp_context, wcmsg->simple_nack.type, wcmsg->simple_nack.id,
                 wcmsg->simple_nack.index);
    } else {
        // wcmsg->type == MESSAGE_VIDEO_BITARRAY_NACK
        BitArray *bit_arr = bit_array_create(wcmsg->bitarray_nack.numBits);
        bit_array_clear_all(bit_arr);

        memcpy(bit_array_get_bits(bit_arr), wcmsg->bitarray_nack.ba_raw,
               BITS_TO_CHARS(wcmsg->bitarray_nack.numBits));

        for (int i = wcmsg->bitarray_nack.index; i < wcmsg->bitarray_nack.numBits; i++) {
            if (bit_array_test_bit(bit_arr, i)) {
                udp_nack(&client->udp_context, wcmsg->bitarray_nack.type, wcmsg->bitarray_nack.id,
                         i);
            }
        }
        bit_array_free(bit_arr);
    }

    return 0;
}

static int handle_stream_reset_request_message(whist_server_state *state,
                                               WhistClientMessage *wcmsg) {
    /*
        Handle an IFrame request message

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Request for i-frame found");
    if (wcmsg->stream_reset_data.type == PACKET_VIDEO) {
        // Mark as wanting an iframe, since that will reset the udp video stream
        state->wants_iframe = true;
        state->last_failed_id = wcmsg->stream_reset_data.last_failed_id;
    } else {
        LOG_ERROR("Can't reset the stream for other types of packets");
    }
    return 0;
}

static int handle_quit_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user quit message

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    UNUSED(wcmsg);
    if (start_quitting_client(&state->client) != 0) {
        LOG_ERROR("Failed to start quitting client.");
        return -1;
    }
    LOG_INFO("Client successfully started quitting.");
    return 0;
}

static int handle_init_message(whist_server_state *state, WhistClientMessage *cwcmsg) {
    /*
        Handle a user init message

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Receiving a message time packet");

    WhistDiscoveryRequestMessage wcmsg = cwcmsg->discoveryRequest;

    state->client_os = wcmsg.os;

    whist_error_monitor_set_username(wcmsg.user_email);

    return 0;
}

static int handle_open_url_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a open URL message

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Step 1: Obtain a pointer to the URL string, and measure the length to ensure it is not too
    // long.
    char *received_url = (char *)&fcmsg->url_to_open;
    size_t url_length = strlen(received_url);
    if (url_length > MAX_URL_LENGTH) {
        LOG_WARNING(
            "Attempted to open url of length %zu, which exceeds the max allowed length (%d "
            "characters)\n",
            url_length, MAX_URL_LENGTH);
        return -1;
    }
    LOG_INFO("Received URL to open in new tab");

    // Step 2: Create the command to run on the Mandelbox's terminal to open the received URL in a
    // new tab. To open a new tab with a given url, we can just use the terminal command: `exec
    // google-chrome <insert url here>`. This command, however, needs to be run after by the same
    // user that ran the initial google-chrome command, responsible for starting the browser on the
    // back end. In our case, the user is 'whist', and we can use the run-as-whist-user.sh script to
    // do just that. We pass the `exec google-chrome <received url here>` command as a parameter to
    // the run-as-whist-user.sh script, and the script will take care of the rest.
    const size_t len_cmd_before_url =
        strlen("/usr/share/whist/run-as-whist-user.sh \"exec google-chrome \"");
    // The maximum possible command length is equal to the (constant) length of the part of the
    // command that needs to go before the url plus the length of the url itself, which may be up to
    // MAX_URL_LENGTH.
    char *command = (char *)calloc(url_length + len_cmd_before_url + 1, sizeof(char));
    sprintf(command, "/usr/share/whist/run-as-whist-user.sh \"exec google-chrome %s\"",
            received_url);

    // Step 3: Execute the command created in step 2 (which consists of a call to the
    // run-as-whist-user.sh script with the appropriate parameter) in the mandelbox, and save the
    // resulting stdout in the open_url_result string.
    char *open_url_result;
    int ret = runcmd(command, &open_url_result);
    if (ret != 0) {
        LOG_ERROR("Error opening URL in new tab: %s", open_url_result);
        free(open_url_result);
        return -1;
    }

    free(open_url_result);
    free(command);

    return 0;
}
