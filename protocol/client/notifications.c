/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.c
 * @brief Contains utilities to receive notifications from the server and display them.

Implements logic to extract notification details from a WhistPacket and send it
to the native notification displayer.

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
#include "notif_utils/display_notifs.h"

/*
============================
Public Functions
============================
*/

int display_notification(WhistPacket *packet);

/*
============================
Public Function Implementations
============================
*/

int display_notification(WhistPacket *packet) {
    // TODO(kmeng01) catch case where packet is not notification data
    WhistNotification c;
    memcpy(c.title, packet->data, MAX_NOTIF_TITLE_LEN);
    memcpy(c.message, (packet->data) + MAX_NOTIF_TITLE_LEN, MAX_NOTIF_MSG_LEN);

    return native_show_notification(c.title, c.message);
}
