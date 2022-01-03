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
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include <dbus/dbus.h>

#include <whist/logging/logging.h>
#include <whist/core/whist.h>
#include <whist/utils/whist_notification.h>
#include "client.h"
#include "network.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief Connects to the d-bus daemon and begins listening for notifications.
 * When notifications arrive, they are sent to the client via the Whist protocol.
 * 
 */
int init_notif_watcher();

#endif  // NOTIFICATIONS_H
