#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.h
 * @brief Contains utilities to capture and send notifications to the client.
============================
Usage
============================

TODO(kmeng01) write some summary thing here

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
