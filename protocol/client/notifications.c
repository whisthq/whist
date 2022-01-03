/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file notifications.c
 * @brief Contains utilities to receive notifications from the server and display them.
============================
Usage
============================

TODO(kevin) write some summary thing here

*/

/*
============================
Includes
============================
*/

#include "notifications.h"

#include <stdio.h>
#include <string.h>

#include <whist/logging/logging.h>
#include <whist/network/network.h>
#include <whist/utils/whist_notification.h>
#include <beeep-wrapper/beeep-wrapper.h>

/*
============================
Public Function Implementations
============================
*/

int display_notification(WhistPacket *packet) {
    LOG_INFO("GOT PACKET: %s", packet->data);

    WhistNotification c;
    memcpy(c.title, packet->data, MAX_NOTIF_TITLE_LEN);
    memcpy(c.message, (packet->data) + MAX_NOTIF_TITLE_LEN, MAX_NOTIF_MSG_LEN);

    ShowNotification(c.title, c.message, NULL);

    FILE *fout = fopen("/Users/kevin/Downloads/foobar.txt", "w");
    fprintf(fout, "title %s\nmessage %s\n", c.title, c.message);
    fclose(fout);

    return 0;
}
