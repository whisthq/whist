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

struct event_base* notifs_evbase = event_base_new();
init_notifications_thread(&server_state, notifs_evbase);

// execute main loop of server

destroy_notifications_thread(notifs_evbase);

*/

/*
============================
Includes
============================
*/

#include <event.h>
#include "state.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief           Initializes a notification processing thread.
 *
 * @param state     Server state. Must be specified to interface with the Whist protocol.
 * @param eb        An event base to handle d-bus message listening.
 */
void init_notifications_thread(whist_server_state *state, struct event_base *eb);

/**
 * @brief       Destroys a notification processing thread by closing connections
 *              and freeing memory.
 *
 * @param eb    An event base object (libevent). Used to identify and break the
 *              libevent event loop.
 */
void destroy_notifications_thread(struct event_base *eb);

#endif  // NOTIFICATIONS_H
