/**
 * Copyright 2022 Whist Technologies, Inc.
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
Public Function Implementations
============================
*/

int display_notification(WhistPacket *packet) {
    WhistNotification c;
    memcpy(c.title, packet->data, MAX_NOTIF_TITLE_LEN);
    memcpy(c.message, (packet->data) + MAX_NOTIF_TITLE_LEN, MAX_NOTIF_MSG_LEN);

    return native_show_notification(c.title, c.message);
}
