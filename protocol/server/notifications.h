#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file notifications.h
 * @brief Contains utilities to capture and send notifications to the client.
============================
Usage
============================

In the main function of the server, one can create a thread and destroy it later.

NotificationsHandler* init_notifications_handler(whist_server_state* server_state);
// execute main loop of server
destroy_notifications_handler(NotificationsHandler* notifications_handler);

*/

/*
============================
Includes
============================
*/

#include "state.h"

/*
============================
Defines
============================
*/

typedef struct NotificationsHandler NotificationsHandler;

/*
============================
Public Functions
============================
*/

/**
 * @brief                   Initializes a notification processing handler
 *
 * @param server_state      Server state. Must be specified to interface with the Whist protocol.
 * @return                  Notification handler struct containing thread and event base variables.
 */
NotificationsHandler *init_notifications_handler(whist_server_state *server_state);

/**
 * @brief               Destroys a notification processing thread by closing connections
 *                      and freeing memory.
 *
 * @param handler       A notification handler struct.
 */
void destroy_notifications_handler(NotificationsHandler *handler);

#endif  // NOTIFICATIONS_H
