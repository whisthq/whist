/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file display_notifs_windows.c
 * @brief Handles client-side notification display on Windows

Interfaces with native libraries to display notifications thrown from the server application.

*/

/*
============================
Includes
============================
*/

#include "display_notifs.h"

#include <whist/logging/logging.h>

/*
============================
Public Functions
============================
*/

int native_show_notification(WhistPacket *packet);

/*
============================
Public Function Implementations
============================
*/

int native_show_notification(WhistPacket *packet) {
    LOG_WARNING("Notification display not implemented on windows");
    return -1;
}
