#ifndef DISPLAY_NOTIFS_H
#define DISPLAY_NOTIFS_H

/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file display_notifs.h
 * @brief Handles client-side notification display. Integrated with different
 * .c files to display on different platforms
============================
Usage
============================

Interfaces with native libraries on various operating systems (OSX/Windows/Linux)
to display notifications thrown from the server application.

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
