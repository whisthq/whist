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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <dbus/dbus.h>
#include <event.h>

#include <whist/logging/logging.h>
#include <whist/core/whist.h>
#include <whist/utils/whist_notification.h>
#include <whist/utils/threads.h>
#include "client.h"
#include "network.h"
#include "state.h"

/*
============================
Public Functions
============================
*/

int32_t listen_and_process_notifications(void *opaque);

// /**
//  * @brief Connects to the d-bus daemon and prepares to listen for notifications
//  * 
//  * @param eb An eventlib event_base
//  * @return struct dbus_ctx* 
//  */
// struct dbus_ctx *dbus_init(struct event_base *eb, Client *init_server_state_client);

// /**
//  * @brief Frees a complete d-bus context variable
//  * 
//  * @param ctx D-Bus connection context
//  */
// void dbus_close(struct dbus_ctx *ctx);

#endif  // NOTIFICATIONS_H
