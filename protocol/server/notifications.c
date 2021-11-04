/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file notifications.c
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

#include "notifications.h"

/*
============================
Private data
============================
*/

// File and watch descriptors for inotify
int notifications_log_fd;
int notifications_watch_descriptor;

// Stealing comments from inotify manual...
/* Some systems cannot read integer variables if they are not
              properly aligned. On other systems, incorrect alignment may
              decrease performance. Hence, the buffer used for reading from
              the inotify file descriptor should have the same alignment as
              struct inotify_event. */
char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
const struct inotify_event *event;

/*
============================
Public function implementations
============================
*/
int inititialze_notification_watcher() {
    // Initialize using inotify_init1 so we can set the fd as non-blocking
    notifications_log_fd = inotify_init1(IN_NONBLOCK);
    if (notifications_log_fd < 0) {
        LOG_FATAL(
            "notifications::initialize_notification_watcher: failed to initialize notification "
            "watcher");
        return -1;
    }

    // Initialize the watch descriptor to tell us whenever the log file has been written to
    notifications_watch_descriptor = inotify_add_watch(
        notifications_log_fd, CHROMIUM_NOTIFICATION_LOG, IN_CLOSE_WRITE);
    if (notifications_watch_descriptor < 0) {
        LOG_INFO("NOTIFCATIONS: Errno: %s", strerror(errno));
        LOG_FATAL(
            "notifications::initialize_notification_watcher: failed to add watch to notification "
            "log");
        return -1;
    }

    LOG_INFO("NOTIFCATIONS: Sucessfully initialzed the notification watcher");

    return 0;
}

int check_for_notifications() {

    // Since this is non-blocking, if there are no notifications, this
    // will return a len of 0, so we can use this instead of polling
    ssize_t len = read(notifications_log_fd, buf, sizeof(buf));
    LOG_INFO("NOTIFICATIONS: read len: %d", len);
    // EAGAIN is set when we call read and there's no data. Since this is
    // non-blocking (as set in init_inotify1), this is fine behaviour
    if(len == -1 && errno != EAGAIN) {
        LOG_FATAL("notifications::notifications_available: read failed!");
        return -1;
    }

    // Case where there is no new data
    if(len <= 0)
        return 0;
    
    // Case we have data
    return 1;
}

int process_notifications() {
    LOG_INFO("NOTIFICATIONS: printing notification");
    return 0;
}
