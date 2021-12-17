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
static int handle_bitrate_message(whist_server_state *state, WhistClientMessage *wcmsg);
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
    static clock temp_clock;

    switch (wcmsg->type) {
        case MESSAGE_KEYBOARD:
        case MESSAGE_MOUSE_BUTTON:
        case MESSAGE_MOUSE_WHEEL:
        case MESSAGE_MOUSE_MOTION:
        case MESSAGE_MULTIGESTURE:
            start_timer(&temp_clock);
            int r = handle_user_input_message(state, wcmsg);
            log_double_statistic(CLIENT_HANDLE_USERINPUT_TIME,
                                 get_timer(temp_clock) * MS_IN_SECOND);
            return r;
        case MESSAGE_KEYBOARD_STATE:
            return handle_keyboard_state_message(state, wcmsg);
        case MESSAGE_START_STREAMING:
        case MESSAGE_STOP_STREAMING:
            return handle_streaming_toggle_message(state, wcmsg);
        case MESSAGE_MBPS:
            return handle_bitrate_message(state, wcmsg);
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
                "Unknown WhistClientMessage Received. "
                "(Type: %d)",
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
    } else {
        LOG_WARNING("Received streaming message to %s streaming, but we're already in that state!",
                    state->stop_streaming ? "stop" : "start");
        return -1;
    }
    return 0;
}

static int handle_bitrate_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user bitrate change message and update MBPS.

        NOTE: idk how to handle this

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("MSG RECEIVED FOR MBPS: %f/%f/%f", wcmsg->bitrate_data.bitrate / 1024.0 / 1024.0,
             wcmsg->bitrate_data.burst_bitrate / 1024.0 / 1024.0,
             wcmsg->bitrate_data.fec_packet_ratio);
    // Clamp the bitrates, preferring to clamp at MAX
    wcmsg->bitrate_data.bitrate =
        min(max(wcmsg->bitrate_data.bitrate, MINIMUM_BITRATE), MAXIMUM_BITRATE);
    wcmsg->bitrate_data.burst_bitrate =
        min(max(wcmsg->bitrate_data.burst_bitrate, MINIMUM_BURST_BITRATE), MAXIMUM_BURST_BITRATE);
    LOG_INFO("Clamped to %f/%f", wcmsg->bitrate_data.bitrate / 1024.0 / 1024.0,
             wcmsg->bitrate_data.burst_bitrate / 1024.0 / 1024.0);
    // Set the new bitrate data (for the video encoder)
    state->max_bitrate = wcmsg->bitrate_data.bitrate;

    // Update the UDP Context's burst bitrate and fec ratio
    udp_update_bitrate_settings(&state->client.udp_context, wcmsg->bitrate_data.burst_bitrate,
                                wcmsg->bitrate_data.fec_packet_ratio);

    // Update the encoder using the new bitrate
    state->update_encoder = true;
    return 0;
}

static int handle_ping_message(Client *client, WhistClientMessage *wcmsg) {
    /*
        Handle a client ping (alive) message.

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    LOG_INFO("Ping Received - Ping ID %d", wcmsg->ping_data.id);

    // Update ping timer
    start_timer(&client->last_ping);

    // Send pong reply
    WhistServerMessage fsmsg_response = {0};
    fsmsg_response.type = MESSAGE_PONG;
    fsmsg_response.ping_id = wcmsg->ping_data.id;
    timestamp_us server_time = current_time_us();
    whist_lock_mutex(client->timestamp_mutex);
    client->last_ping_client_time = wcmsg->ping_data.original_timestamp;
    client->last_ping_server_time = server_time;
    whist_unlock_mutex(client->timestamp_mutex);

    if (send_packet(&client->udp_context, PACKET_MESSAGE, (uint8_t *)&fsmsg_response,
                    sizeof(fsmsg_response), 1) < 0) {
        LOG_WARNING("Failed to send UDP pong");
        return -1;
    }

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
    LOG_INFO("Request to use codec %d / dimensions %dx%d / dpi %d received",
             wcmsg->dimensions.codec_type, wcmsg->dimensions.width, wcmsg->dimensions.height,
             wcmsg->dimensions.dpi);
    if (state->client_width != wcmsg->dimensions.width ||
        state->client_height != wcmsg->dimensions.height ||
        state->client_codec_type != wcmsg->dimensions.codec_type ||
        state->client_dpi != wcmsg->dimensions.dpi) {
        state->client_width = wcmsg->dimensions.width;
        state->client_height = wcmsg->dimensions.height;
        state->client_codec_type = wcmsg->dimensions.codec_type;
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

    static int local_directory_id = 0;  // static so only set to 0 on first iteration
    static const char *parent_drag_drop_directory = "/home/whist/.teleport/drag-drop";
    size_t target_directory_maxlen =
        strlen(parent_drag_drop_directory) + 64 + 1;  // have enough space for int in paths

    // Use `unique_id` for the rest of the function, NOT `local_directory_id`.
    //     We increment `local_directory_id` here instead of later because if the function
    //     exits early with failure, we still want the next directory ID to be incremented.
    int unique_id = local_directory_id;
    local_directory_id++;

    // Get the path for the `/home/whist/.teleport/drag-drop/downloads/[ID]/` directory
    char *downloads_directory = safe_malloc(target_directory_maxlen);
    memset(downloads_directory, 0, target_directory_maxlen);
    snprintf(downloads_directory, target_directory_maxlen, "%s/downloads/%d",
             parent_drag_drop_directory, unique_id);

    // Get the path for the `/home/whist/.teleport/drag-drop/file_info/[ID]/` directory
    char *file_info_directory = safe_malloc(target_directory_maxlen);
    memset(file_info_directory, 0, target_directory_maxlen);
    snprintf(file_info_directory, target_directory_maxlen, "%s/file_info/%d",
             parent_drag_drop_directory, unique_id);

    // Create file info directory
    if (mkdir(file_info_directory, 0777) == -1) {
        LOG_ERROR("Could not create directory %s for download file info", file_info_directory);
    } else {
        // Get the path for the `/home/whist/.teleport/drag-drop/file_info/[ID]/file_size` file
        char *file_size_file_path = safe_malloc(target_directory_maxlen);
        memset(file_size_file_path, 0, target_directory_maxlen);
        snprintf(file_size_file_path, target_directory_maxlen, "%s/file_size", file_info_directory);

        // Write the file size to file
        FILE *file_size_file_handle = fopen(file_size_file_path, "w");
        if (file_size_file_handle) {
            char file_size_string[64];
            memset(file_size_string, 0, 64);
            snprintf(file_size_string, 64, "%d", wcmsg->file_metadata.file_size);
            fwrite(file_size_string, 1, strlen(file_size_string), file_size_file_handle);
            fclose(file_size_file_handle);
        }

        free(file_size_file_path);
    }

    // Create the downloads subdirectory for this ID
    if (mkdir(downloads_directory, 0777) == -1) {
        LOG_ERROR("Could not create directory %s for download", downloads_directory);
        free(downloads_directory);
        return -1;
    }

    file_synchronizer_open_file_for_writing(downloads_directory, &wcmsg->file_metadata, unique_id);

    free(downloads_directory);

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

    LOG_INFO("Request for i-frame found: Creating iframe");
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

    error_monitor_set_username(wcmsg.user_email);

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
    char *received_url = fcmsg->url_to_open;
    size_t url_length = strlen(received_url);
    if (url_length > MAX_URL_LENGTH) {
        LOG_WARNING(
            "Attempted to open url of length %lu, which exceeds the max allowed length (%lu "
            "characters)\n",
            url_length, MAX_URL_LENGTH);
        return -1;
    }
    LOG_INFO("Received URL to open in new tab: %s", received_url);

    // Step 2: Create the command to run on the Mandelbox's terminal to open the received URL in a
    // new tab. To open a new tab with a given url, we can just use the terminal command: `exec
    // google-chrome <insert url here>`. This command, however, needs to be run after by the same
    // user that ran the initial google-chrome command, responsible for starting the browser on the
    // back end. In our case, the user is 'whist', and we can use the run-as-whist-user.sh script to
    // do just that. We pass the `exec google-chrome <received url here>` command as a parameter to
    // the run-as-whist-user.sh script, and the script will take care of the rest.
    size_t len_cmd_before_url =
        strlen("/usr/share/whist/run-as-whist-user.sh \"exec google-chrome \"");
    size_t command_len = len_cmd_before_url + url_length + 1;
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
        free(open_url_result);
        LOG_ERROR("Running command to open URL %s resulted in error: %s", received_url,
                  open_url_result);
        return -1;
    }

    free(open_url_result);
    free(command);

    return 0;
}
