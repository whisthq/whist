#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file notifications.h
 * @brief This file contains the main code that runs a Whist server on a
 *        Windows or Linux Ubuntu computer.
============================
Usage
============================

TODO

*/

/*
============================
Includes
============================
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dbus/dbus.h>
#include <fractal/logging/logging.h>

/** The bus name used to talk to the bus itself. */
#define DBUS_SERVICE_DBUS      "org.freedesktop.DBus"

/** The object path used to talk to the bus itself. */
#define DBUS_PATH_DBUS  "/org/freedesktop/DBus"

/** The monitoring interface exported by the dbus-daemon */
#define DBUS_INTERFACE_MONITORING     "org.freedesktop.DBus.Monitoring"

/** A match rule that enables eavesdropping, AKA to allow our
 * connection to monitor the messages between other clients and
 * the DBUS server */
#define EAVESDROPPING_RULE "eavesdrop=true"

/**
 * Just a convience macro for converting integer to an int pointer
 */
#define _DBUS_INT_TO_POINTER(integer) ((void*)((intptr_t)(integer)))

/** The filter we will add to our DBUS monitor connection. This
 * will allow us to only get messages from the Notifications interface */
#define NOTIFICATION_FILTER "interface='org.freedesktop.Notifications'"

typedef enum {
    BINARY_MODE_NOT,
    BINARY_MODE_RAW,
    BINARY_MODE_PCAP
} BinaryMode;


/**
 * @brief Initializes the connection to the notification DBUS
 * 
 * @return 0 on success, -1 on failure
 */
int inititialze_notification_dbus();

/**
 * @brief Checks for messages from the notification DBUS. If they
 * are available, we will asynchronously parse them and send those packets over to 
 * the client
 * 
 * @return 0 on success, -1 on failure
 */
int process_notification_queue();

#endif // NOTIFICATIONS_H