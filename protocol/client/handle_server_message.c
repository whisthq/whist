/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include <whist/core/whist.h>
#include <whist/utils/clock.h>
#include <whist/logging/logging.h>
#include <whist/logging/log_statistic.h>
#include <whist/utils/window_info.h>
#include "client_utils.h"
#include "audio.h"
#include "network.h"
#include "sdl_utils.h"
#include "display_notifs.h"
#include "client_statistic.h"

#include <stddef.h>

bool client_exiting = false;
extern int audio_frequency;
extern volatile double latency;
extern volatile int try_amount;

/*
============================
Private Functions
============================
*/

static int handle_tcp_pong_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_quit_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_clipboard_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_window_title_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_open_uri_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_fullscreen_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_file_metadata_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_file_chunk_message(WhistServerMessage *wsmsg, size_t wsmsg_size);
static int handle_notification_message(WhistServerMessage *wsmsg, size_t wsmsg_size);

/*
============================
Private Function Implementations
============================
*/

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int handle_server_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle message packets from the server

        Arguments:
            wsmsg (WhistServerMessage*): server message packet
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    switch (wsmsg->type) {
        case MESSAGE_TCP_PONG:
            return handle_tcp_pong_message(wsmsg, wsmsg_size);
        case SMESSAGE_QUIT:
            return handle_quit_message(wsmsg, wsmsg_size);
        case SMESSAGE_CLIPBOARD:
            return handle_clipboard_message(wsmsg, wsmsg_size);
        case SMESSAGE_WINDOW_TITLE:
            return handle_window_title_message(wsmsg, wsmsg_size);
        case SMESSAGE_OPEN_URI:
            return handle_open_uri_message(wsmsg, wsmsg_size);
        case SMESSAGE_FULLSCREEN:
            return handle_fullscreen_message(wsmsg, wsmsg_size);
        case SMESSAGE_FILE_DATA:
            return handle_file_chunk_message(wsmsg, wsmsg_size);
        case SMESSAGE_FILE_METADATA:
            return handle_file_metadata_message(wsmsg, wsmsg_size);
        case SMESSAGE_NOTIFICATION:
            return handle_notification_message(wsmsg, wsmsg_size);
        default:
            LOG_WARNING("Unknown WhistServerMessage Received (type: %d)", wsmsg->type);
            return -1;
    }
}

static int handle_tcp_pong_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle server TCP pong message

        Arguments:
            wsmsg (WhistServerMessage*): server TCP pong message
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    if (wsmsg_size != sizeof(WhistServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: TCP pong message)!");
        return -1;
    }
    receive_tcp_pong(wsmsg->ping_id);
    return 0;
}

static int handle_quit_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle server quit message

        Arguments:
            wsmsg (WhistServerMessage*): server quit message
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    UNUSED(wsmsg);
    if (wsmsg_size != sizeof(WhistServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: quit message)!");
        return -1;
    }
    LOG_INFO("Server signaled a quit!");
    client_exiting = true;
    return 0;
}

static int handle_clipboard_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle server clipboard message

        Arguments:
            wsmsg (WhistServerMessage*): server clipboard message
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    if (wsmsg_size != sizeof(WhistServerMessage) + wsmsg->clipboard.size) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: clipboard message)! Expected %zu, but received %zu",
            sizeof(WhistServerMessage) + wsmsg->clipboard.size, wsmsg_size);
        return -1;
    }
    LOG_INFO("Received %zu byte clipboard message from server!", wsmsg_size);
    // Known to run in less than ~100 assembly instructions
    push_clipboard_chunk(&wsmsg->clipboard);
    return 0;
}

static int handle_window_title_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle server window title message
        Since only the main thread is allowed to perform UI functionality on MacOS, instead of
        calling SDL_SetWindowTitle directly, this function updates a global variable window_title.
        The main thread periodically polls this variable to determine if it needs to update the
        window title.

        Arguments:
            wsmsg (WhistServerMessage*): server window title message
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    LOG_INFO("Received window title message from server!");

    sdl_set_window_title((char *)wsmsg->window_title);

    return 0;
}

static int handle_open_uri_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle server open URI message by launching the relevant URI locally

        Arguments:
            wsmsg (WhistServerMessage*): server open uri message
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */
    LOG_INFO("Received Open URI message from the server!");

#if defined(_WIN32)
#define OPEN_URI_CMD "cmd /c start \"\""
#elif __APPLE__
#define OPEN_URI_CMD "open"
#else
#define OPEN_URI_CMD "xdg-open"
#endif  // _WIN32
// just to be safe from off-by-1 errors
#define OPEN_URI_CMD_MAXLEN 30

    const char *uri = (const char *)&wsmsg->requested_uri;
    const int cmd_len = (int)strlen(uri) + OPEN_URI_CMD_MAXLEN + 1;
    char *cmd = safe_malloc(cmd_len);
    memset(cmd, 0, cmd_len);
    snprintf(cmd, cmd_len, OPEN_URI_CMD " \"%s\"", uri);
    runcmd(cmd, NULL);
    free(cmd);
    return 0;
}

static int handle_fullscreen_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle server fullscreen message (enter or exit events)

        Arguments:
            wsmsg (WhistServerMessage*): server fullscreen message
            wsmsg_size (size_t): size of the packet message contents

        Return:
            (int): 0 on success, -1 on failure
    */

    LOG_INFO("Received fullscreen message from the server! Value: %d", wsmsg->fullscreen);

    sdl_set_fullscreen(wsmsg->fullscreen);

    return 0;
}

static int handle_file_metadata_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle a file metadata message.

        Arguments:
            wsmsg (WhistServerMessage*): message packet from client
        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    file_synchronizer_open_file_for_writing(&wsmsg->file_metadata);

    return 0;
}

static int handle_file_chunk_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle a file chunk message.

        Arguments:
            wsmsg (WhistServerMessage*): message packet from client
        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    file_synchronizer_write_file_chunk(&wsmsg->file);

    return 0;
}

static int handle_notification_message(WhistServerMessage *wsmsg, size_t wsmsg_size) {
    /*
        Handle a file chunk message.

        Arguments:
            wsmsg (WhistServerMessage*): message packet from client
        Returns:
            (int): Returns -1 on failure, 0 on success
    */

    native_show_notification(wsmsg->notif.title, wsmsg->notif.message);
    log_double_statistic(NOTIFICATIONS_RECEIVED, 1.);

    return 0;
}
