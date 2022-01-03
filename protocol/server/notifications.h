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

#include <stdint.h>

/*
============================
Public Functions
============================
*/

int32_t listen_and_process_notifications(void *opaque);

#endif  // NOTIFICATIONS_H
