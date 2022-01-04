/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.c
 * @brief Contains utilities to receive notifications from the server and display them.
============================
Usage
============================

TODO(kevin) write some summary thing here

*/

/*
============================
Includes
============================
*/

#include "notifications.h"

#include <string.h>

#include <whist/logging/logging.h>
#include <whist/network/network.h>
#include <whist/utils/whist_notification.h>
#include "display_notifs.h"

/*
============================
Public Function Implementations
============================
*/

int display_notification(WhistPacket *packet) {
    LOG_INFO("GOT PACKET: %s", packet->data);

    WhistNotification c;
    memcpy(c.title, packet->data, MAX_NOTIF_TITLE_LEN);
    memcpy(c.message, (packet->data) + MAX_NOTIF_TITLE_LEN, MAX_NOTIF_MSG_LEN);

    if (!notif_bundle_initialized()) {
        if (init_notif_bundle() < 0) {
            LOG_FATAL("MacOS notification setup failed");
        }
    }

    return deliver_notification(c.title, c.message);
}
