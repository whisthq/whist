/**
 * Copyright 2022 Whist Technologies, Inc.
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
Public Function Implementations
============================
*/

int display_notification(char *title, char *msg) {
    LOG_WARNING("Notification display not implemented on windows");
    return -1;
}
