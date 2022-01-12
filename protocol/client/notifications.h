#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file notifications.h
 * @brief Contains utilities to receive notifications from the server and display them.
============================
Usage
============================

After receiving a WhistPacket consisting of notification data, the client code can call
display_notification to natively the notification natively on the user's computer.

*/

/*
============================
Includes
============================
*/

#include <whist/network/network.h>

/*
============================
Public Functions
============================
*/

/**
 * @brief           Displays notification contained in WhistPacket
 *
 * @param packet    WhistPacket containing notification data
 * @return int      0 if success, -1 if failure
 */
int display_notification(WhistPacket *packet);

#endif  // NOTIFICATIONS_H
