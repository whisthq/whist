#ifndef WHIST_NOTIFICATION_H
#define WHIST_NOTIFICATION_H

/**
 * Copyright 2021 Whist Technologies, Inc., dba Whist
 * @file whist_notification.h
 * @brief Contains a notification struct for convenient data packaging
============================
Usage
============================

TODO(kevin) write some summary thing here

*/

/*
============================
Globals
============================
*/

// Arbitrarily chosen
#define MAX_NOTIF_TITLE_LEN 500
#define MAX_NOTIF_MSG_LEN 500

typedef struct WhistNotification {
    char title[MAX_NOTIF_TITLE_LEN];
    char message[MAX_NOTIF_MSG_LEN];
} WhistNotification;

#endif  // WHIST_NOTIFICATION_H
