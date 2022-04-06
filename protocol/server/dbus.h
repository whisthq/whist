#ifndef WHIST_SERVER_DBUS_H
#define WHIST_SERVER_DBUS_H

/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file dbus.h
 * @brief Contains utilities to capture and handle D-Bus messages.
 *
============================
Usage
============================

In the main function of the server, one can create a thread and destroy it later.

DBusHandlerContext* dbus_handler = whist_create_dbus_handler(Wserver_state);
// execute main loop of server
whist_destroy_dbus_handler(dbus_handler);

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

typedef struct DBusHandlerContext DBusHandlerContext;

/*
============================
Public Functions
============================
*/

/**
 * @brief                   Initializes a D-Bus handling thread
 *
 * @param server_state      Server state. Must be specified to interface with the Whist protocol.
 *
 * @return                  A reference to the created D-Bus handler context.
 */
DBusHandlerContext* whist_create_dbus_handler(WhistServerState* server_state);

/**
 * @brief               Destroys a D-Bus handling thread by closing connections
 *                      and freeing memory.
 *
 * @param ctx           A D-Bus handler context to free.
 */
void whist_destroy_dbus_handler(DBusHandlerContext* ctx);

#endif  // WHIST_SERVER_DBUS_H
