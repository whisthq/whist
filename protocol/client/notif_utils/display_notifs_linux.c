/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file display_notifs_linux.c
 * @brief Handles client-side notification display on Linux

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
Public Function Implementations
============================
*/

int native_show_notification(WhistPacket *packet) {
    LOG_WARNING("Notification display not implemented on linux");
    return -1;
}
