#ifndef OSUTILS_H
#define OSUTILS_H

/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file os_utils.h
============================
Usage
============================
*/

/*
============================
Defines
============================
*/

#define WHIST_KB_LAYOUT_NAME_MAX_LENGTH 24
#define MAX_ADDITIONAL_COMMAND_LENGTH 100

typedef struct {
    char layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH];
    char additional_command[MAX_ADDITIONAL_COMMAND_LENGTH];
} WhistKeyboardLayout;

// Hand-chosen numbers that approximate (slightly overestimate) the maximum
// display size of an OSX notification. Strings are truncated at these lengths.
#define MAX_NOTIF_TITLE_LEN 50
#define MAX_NOTIF_MSG_LEN 150

typedef struct WhistNotification {
    char title[MAX_NOTIF_TITLE_LEN];
    char message[MAX_NOTIF_MSG_LEN];
} WhistNotification;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Get keyboard layout
 *
 * @returns                        The current keyboard layout
 */
WhistKeyboardLayout get_keyboard_layout(void);

/**
 * @brief                          Set keyboard layout
 *
 * @param requested_layout         The layout we're requesting the OS to use
 */
void set_keyboard_layout(WhistKeyboardLayout requested_layout);

/**
 * @brief       Displays notification on the client side.
 *              Specific implementation depends on operating system (OSX/Windows/Linux).
 *
 * @param notif A WhistNotification: contains the content of a notification.
 *
 * @return int  0 if success, -1 if failure
 *
 * @note This function is ONLY implemented for MacOS. Linux and Windows impls are stubs.
 */
int display_notification(WhistNotification notif);

/**
 * @brief               Package title and message strings into notification.
 *
 * @param notif         Pointer to notification.
 * @param title         Notification title.
 * @param message       Notification message.
 */
void package_notification(WhistNotification *notif, const char *title, const char *message);

#endif
