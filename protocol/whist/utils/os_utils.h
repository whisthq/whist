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

typedef struct {
    char layout_name[WHIST_KB_LAYOUT_NAME_MAX_LENGTH];
} WhistKeyboardLayout;

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
 * @param title Notification title
 * @param msg   Notification body
 *
 * @return int  0 if success, -1 if failure
 *
 * @note This function is ONLY implemented for MacOS. Linux and Windows impls are stubs.
 */
int display_notification(char *title, char *msg);

#endif
