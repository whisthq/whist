/**
 * Copyright Fractal Computers, Inc. 2020
 * @file handle_server_message.c
 * @brief This file contains all the code for client-side processing of messages
 *        received from the server
============================
Usage
============================

handleServerMessage() must be called on any received message from the server.
Any action trigged a server message must be initiated in network.c.
*/

/*
============================
Includes
============================
*/

#include "handle_server_message.h"

#include <stddef.h>

#include <fractal/core/fractal.h>
#include <fractal/utils/clock.h>
#include <fractal/utils/logging.h>
#include <fractal/utils/window_name.h>
#include "client_utils.h"
#include "audio.h"

#include <stddef.h>

extern bool exiting;
extern int audio_frequency;
extern volatile bool is_timing_latency;
extern volatile double latency;
extern volatile clock latency_timer;
extern volatile int ping_id;
extern volatile int ping_failures;
extern volatile int try_amount;
extern volatile char *window_title;
extern volatile bool should_update_window_title;

/*
============================
Private Functions
============================
*/

static int handle_pong_message(FractalServerMessage *fmsg, size_t fmsg_size);
static int handle_quit_message(FractalServerMessage *fmsg, size_t fmsg_size);
static int handle_audio_frequency_message(FractalServerMessage *fmsg, size_t fmsg_size);
static int handle_clipboard_message(FractalServerMessage *fmsg, size_t fmsg_size);
static int handle_window_title_message(FractalServerMessage *fmsg, size_t fmsg_size);

/*
============================
Private Function Implementations
============================
*/

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int handle_server_message(FractalServerMessage *fmsg, size_t fmsg_size) {
    /*
        Handle message packets from the server

        Arguments:
            fmsg (FractalServerMessage*): server message packet
            fmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    switch (fmsg->type) {
        case MESSAGE_PONG:
            return handle_pong_message(fmsg, fmsg_size);
        case SMESSAGE_QUIT:
            return handle_quit_message(fmsg, fmsg_size);
        case MESSAGE_AUDIO_FREQUENCY:
            return handle_audio_frequency_message(fmsg, fmsg_size);
        case SMESSAGE_CLIPBOARD:
            return handle_clipboard_message(fmsg, fmsg_size);
        case SMESSAGE_WINDOW_TITLE:
            return handle_window_title_message(fmsg, fmsg_size);
        default:
            LOG_WARNING("Unknown FractalServerMessage Received");
            return -1;
    }
}

static int handle_pong_message(FractalServerMessage *fmsg, size_t fmsg_size) {
    /*
        Handle server pong message

        Arguments:
            fmsg (FractalServerMessage*): server pong message
            fmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    if (fmsg_size != sizeof(FractalServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: pong message)!");
        return -1;
    }
    if (ping_id == fmsg->ping_id) {
        double latency_time = get_timer(latency_timer);
        LOG_INFO("Latency: %f", latency_time);
        // Save latency as a geometric sum (Latency per ping capped at 1 second)
        latency = 0.5 * latency + 0.5 * min(latency_time, 1.0);
        is_timing_latency = false;
        ping_failures = 0;
        try_amount = 0;
    } else {
        LOG_INFO("Old Ping ID found: Got %d but expected %d", fmsg->ping_id, ping_id);
    }
    return 0;
}

static int handle_quit_message(FractalServerMessage *fmsg, size_t fmsg_size) {
    /*
        Handle server quit message

        Arguments:
            fmsg (FractalServerMessage*): server quit message
            fmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    UNUSED(fmsg);
    if (fmsg_size != sizeof(FractalServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: quit message)!");
        return -1;
    }
    LOG_INFO("Server signaled a quit!");
    exiting = true;
    return 0;
}

static int handle_audio_frequency_message(FractalServerMessage *fmsg, size_t fmsg_size) {
    /*
        Handle server audio frequency message

        Arguments:
            fmsg (FractalServerMessage*): server audio frequency message
            fmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    if (fmsg_size != sizeof(FractalServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: audio frequency message)!");
        return -1;
    }
    LOG_INFO("Changing audio frequency to %d", fmsg->frequency);
    set_audio_frequency(fmsg->frequency);
    return 0;
}

static int handle_clipboard_message(FractalServerMessage *fmsg, size_t fmsg_size) {
    /*
        Handle server clipboard message

        Arguments:
            fmsg (FractalServerMessage*): server clipboard message
            fmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    if (fmsg_size != sizeof(FractalServerMessage) + fmsg->clipboard.size) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: clipboard message)! Expected %d, but received %d",
            sizeof(FractalServerMessage) + fmsg->clipboard.size, fmsg_size);
        return -1;
    }
    LOG_INFO("Received %d byte clipboard message from server!", fmsg_size);
    // Known to run in less than ~100 assembly instructions
    if (!clipboard_synchronizer_set_clipboard_chunk(&fmsg->clipboard)) {
        LOG_ERROR("Failed to set local clipboard from server message.");
        return -1;
    }
    return 0;
}

static int handle_window_title_message(FractalServerMessage *fmsg, size_t fmsg_size) {
    /*
        Handle server window title message
        Since only the main thread is allowed to perform UI functionality on MacOS, instead of
        calling SDL_SetWindowTitle directly, this function updates a global variable window_title.
        The main thread periodically polls this variable to determine if it needs to update the
        window title.

        Arguments:
            fmsg (FractalServerMessage*): server window title message
            fmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    LOG_INFO("Received window title message from server!");
    if (should_update_window_title) {
        LOG_ERROR(
            "Failed to update window title, as the previous window title update is still pending");
        return -1;
    }

    char *title = (char *)&fmsg->window_title;
    size_t len = strlen(title) + 1;
    char *new_window_title = safe_malloc(len);
    safe_strncpy(new_window_title, title, strlen(title) + 1);
    window_title = new_window_title;

    should_update_window_title = true;
    return 0;
}
