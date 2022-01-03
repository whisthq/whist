#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.h
 * @brief Contains utilities to capture and send notifications for native display in the client.
============================
Usage
============================

write some summary thing here

*/

/*
============================
Includes
============================
*/

#include <stdio.h>

#include <whist/network/network.h>

/*
============================
Public Functions
============================
*/

int display_notification(WhistPacket *packet);

#endif  // NOTIFICATIONS_H
