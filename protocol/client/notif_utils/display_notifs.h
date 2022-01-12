#ifndef DISPLAY_NOTIFS_H
#define DISPLAY_NOTIFS_H

/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file display_notifs.h
 * @brief Handles client-side notification display. Integrated with different
 * .c files to display on different platforms
============================
Usage
============================

Interfaces with native libraries on various operating systems (OSX/Windows/Linux)
to display notifications thrown from the server application.

Example usage: displaying information from a whist packet

int display_notification(WhistPacket *packet) {
    WhistNotification c;
    memcpy(c.title, packet->data, MAX_NOTIF_TITLE_LEN);
    memcpy(c.message, (packet->data) + MAX_NOTIF_TITLE_LEN, MAX_NOTIF_MSG_LEN);

    return native_show_notification(c.title, c.message);
}

*/

/*
============================
Public Functions
============================
*/

/**
 * @brief       Displays notification on the client side.
 *              Specific implementation depends on operating system (OSX/Windows/Linux).
 *
 * @param title Notification title
 * @param msg   Notification body
 *
 * @return int  0 if success, -1 if failure
 *
 * @note This function is ONLY implemented for MacOS. Linux and Windows impls are stubs.
 */
int native_show_notification(char *title, char *msg);

#endif  // DISPLAY_NOTIFS_H
