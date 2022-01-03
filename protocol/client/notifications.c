/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.c
 * @brief Contains utilities to receive notifications from the server and display them.
============================
Usage
============================

TODO(kevin) write some summary thing here

*/

#include "notifications.h"

#include <stdio.h>

#include <whist/logging/logging.h>
#include <whist/network/network.h>

/*
============================
Public Function Implementations
============================
*/

int display_notification(WhistPacket *packet) {
    LOG_INFO("GOT PACKET: %s\n", packet->data);

    return 0;
}