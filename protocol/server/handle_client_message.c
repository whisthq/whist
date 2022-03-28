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
#include "whist/core/features.h"
#include "state.h"
#include "client.h"
#include "handle_client_message.h"
#include "network.h"

/*
============================
Private Functions
============================
*/

static int handle_user_input_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_keyboard_state_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_streaming_toggle_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_dimensions_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_clipboard_message(WhistClientMessage *wcmsg);
static int handle_quit_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_init_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_file_metadata_message(WhistClientMessage *wcmsg);
static int handle_file_chunk_message(WhistClientMessage *wcmsg);
static int handle_open_urls_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_frame_ack_message(whist_server_state *state, WhistClientMessage *wcmsg);
static int handle_file_upload_cancel_message(whist_server_state *, WhistClientMessage *wcmsg);

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
        case MESSAGE_DIMENSIONS:
            return handle_dimensions_message(state, wcmsg);
        case CMESSAGE_CLIPBOARD:
            return handle_clipboard_message(wcmsg);
        case CMESSAGE_FILE_METADATA:
            return handle_file_metadata_message(wcmsg);
        case CMESSAGE_FILE_DATA:
            return handle_file_chunk_message(wcmsg);
        case CMESSAGE_QUIT:
            return handle_quit_message(state, wcmsg);
        case CMESSAGE_INIT:
            return handle_init_message(state, wcmsg);
        case MESSAGE_OPEN_URL:
            return handle_open_urls_message(state, wcmsg);
        case MESSAGE_FRAME_ACK:
            return handle_frame_ack_message(state, wcmsg);
        case MESSAGE_FILE_UPLOAD_CANCEL:
            return handle_file_upload_cancel_message(state, wcmsg);
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
        state->stream_needs_restart = true;
    } else {
        LOG_WARNING("Received streaming message to %s streaming, but we're already in that state!",
                    state->stop_streaming ? "stop" : "start");
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
            "No need to update the encoder as the requested parameters are the same as the "
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

static int handle_quit_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    /*
        Handle a user quit message

        Arguments:
            wcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    UNUSED(wcmsg);
    start_deactivating_client(state->client);
    LOG_INFO("Client QUIT message received...");
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

    LOG_INFO("Received a ClientInitMessage packet.");

    ClientInitMessage init_msg = cwcmsg->init_message;

    state->client_os = init_msg.os;
    whist_error_monitor_set_username(init_msg.user_email);

    return 0;
}

static int handle_open_urls_message(whist_server_state *state, WhistClientMessage *fcmsg) {
    /*
        Handle a open URL message

        Arguments:
            fcmsg (WhistClientMessage*): message package from client

        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    // Step 1: Obtain a pointer to the URL string, and measure the length to ensure it is not too
    // long.
    char *received_urls = (char *)&fcmsg->urls_to_open;
    size_t urls_length = strlen(received_urls);
    if (urls_length > MAX_URL_LENGTH * MAX_NEW_TAB_URLS) {
        LOG_WARNING(
            "Attempted to open url(s) of length %zu, which exceed the max allowed length (%d "
            "characters)\n",
            urls_length, MAX_URL_LENGTH * MAX_NEW_TAB_URLS);
        return -1;
    }
    LOG_INFO("Received URL to open in new tab");

    // Step 2: Split the URLs using the separator '|' character, and wrap the urls in double quotes
    // (escaped twice)
    char *wrapped_urls = (char *)safe_zalloc(
        (MAX_URL_LENGTH + 2 * strlen("\\\"") + strlen(" ")) * MAX_NEW_TAB_URLS * sizeof(char));
    sprintf(wrapped_urls, "\\\"");
    size_t index = strlen("\\\"");
    for (size_t i = 0; i < urls_length; i++) {
        if (received_urls[i] == '|') {
            sprintf(wrapped_urls + index, "\\\" \\\"");
            index += strlen("\\\" \\\"");
        } else {
            wrapped_urls[index] = received_urls[i];
            index += 1;
        }
    }
    sprintf(wrapped_urls + index, "\\\"");
    // Recompute size of string containing urls to take into account the wrapping and splitting
    urls_length = strlen(wrapped_urls);

    // Step 3: Create the command to run on the Mandelbox's terminal to open the received URL in a
    // new tab. To open a new tab with a given url, we can just use the terminal command: `exec
    // /usr/bin/whist-open-new-tab <insert url(s) here>`. This command needs to be run by the
    // `whist` user, so we run it through the run-as-whist-user.sh script.
    const size_t len_cmd_before_urls =
        strlen("/usr/share/whist/run-as-whist-user.sh \"exec /usr/bin/whist-open-new-tab \"");
    // The maximum possible command length is equal to the (constant) length of the part of the
    // command that needs to go before the urls plus the length of the wrapped urls
    char *command = (char *)safe_zalloc((len_cmd_before_urls + urls_length + 1) * sizeof(char));
    sprintf(command,
            "/usr/share/whist/run-as-whist-user.sh \"exec /usr/bin/whist-open-new-tab %s\"",
            wrapped_urls);
    free(wrapped_urls);

    // Step 4: Execute the command created in step 3 in the mandelbox, and save the
    // resulting stdout in the open_urls_result string.
    char *open_urls_result;
    int ret = runcmd(command, &open_urls_result);
    free(command);
    if (ret == -1) {
        LOG_ERROR("Error opening URL in new tab: %s", open_urls_result);
        // No need to free open_urls_result, because the pointer is NULL when ret == -1
        return -1;
    }
    free(open_urls_result);
    return 0;
}

static int handle_frame_ack_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    FATAL_ASSERT(FEATURE_ENABLED(LONG_TERM_REFERENCE_FRAMES) &&
                 "Received frame ack but long-term reference frames "
                 "are not enabled.");

    if (LOG_LONG_TERM_REFERENCE_FRAMES) {
        LOG_INFO("Received frame ack for frame ID %d.", wcmsg->frame_ack.frame_id);
    }

    state->frame_ack_id = wcmsg->frame_ack.frame_id;
    state->update_frame_ack = true;

    return 0;
}

static int handle_file_upload_cancel_message(whist_server_state *state, WhistClientMessage *wcmsg) {
    file_synchronizer_cancel_user_file_upload();
    return 0;
}
