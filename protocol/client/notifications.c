/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file notifications.c
 * @brief This file contains all the code for client-side processing of notifications
 *        received from the server
 **/

/*
============================
Includes
============================
*/

#include "notifications.h"
#include "network.h"
//#include "ringbuffer.h"

int display_notification(FractalPacket* packet) {
    FractalNotification c;
    memcpy(c.title, packet->data, MAX_TITLE_LEN);
    memcpy(c.message, (packet->data)+MAX_TITLE_LEN, MAX_MESSAGE_LEN);
    DisplayNotification(c.title, c.message);
    return 0;
}