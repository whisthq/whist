#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.h
 * @brief Contains utilities to receive notifications from the server and display them.
============================
Usage
============================

TODO(kevin) write some summary thing here

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

int display_notification(WhistPacket *packet);

#endif  // NOTIFICATIONS_H
