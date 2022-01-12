#ifndef WHIST_NOTIFICATION_H
#define WHIST_NOTIFICATION_H

/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file whist_notification.h
 * @brief Contains a notification struct for convenient data packaging
============================
Usage
============================

You can create a notification and copy strings into its fields.
Please ensure you do not access out-of-bounds memory!

char *title = "something", *n_message = "something else";
WhistNotification notif;

strncpy(notif.title, title, MAX_NOTIF_TITLE_LEN - 1);
notif.title[min(strlen(title), MAX_NOTIF_TITLE_LEN - 1)] = '\0';
strncpy(notif.message, n_message, MAX_NOTIF_MSG_LEN - 1);
notif.message[min(strlen(n_message), MAX_NOTIF_MSG_LEN - 1)] = '\0';

*/

/*
============================
Defines
============================
*/

#define MAX_NOTIF_TITLE_LEN 500
#define MAX_NOTIF_MSG_LEN 500

typedef struct WhistNotification {
    char title[MAX_NOTIF_TITLE_LEN];
    char message[MAX_NOTIF_MSG_LEN];
} WhistNotification;

#endif  // WHIST_NOTIFICATION_H
