#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file notifications.h
 * @brief This file contains the code that monitors and processes notifications
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
#include <sys/types.h>
#include <sys/inotify.h>
#include <fractal/logging/logging.h>
#include <fractal/core/fractal.h>
#include <fractal/core/fractal_notification.h>
#include "network.h"
#include "client.h"

// The server executable is in /usr/share/fractal
#define CHROMIUM_NOTIFICATION_LOG \
    "/fractal/userConfigs/google-chrome/Default//Platform Notifications//000003.log"
    
extern Client client;
/**
 * @brief Initializes the connection the notificaiton watcher
 *
 * @return 0 on success, -1 on failure
 */
int inititialze_notification_watcher();

/**
 * @brief Check if notifications are available
 *
 * @return -1 on failure, 0 on false, 1 on true
 */
int check_for_notifications();

/**
 * @brief If notifications are available, then it will parse them and send
 * these to the client
 *
 * @return 0 on success, -1 on failure
 */
int process_notifications();

#endif  // NOTIFICATIONS_H
