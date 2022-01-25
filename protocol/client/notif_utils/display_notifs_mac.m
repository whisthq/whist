/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file display_notifs_mac.m
 * @brief Handles client-side notification display on MacOS

Interfaces with native libraries to display notifications thrown from the server application.

*/

/*
============================
Includes
============================
*/

#import "../display_notifs.h"

#import <stdio.h>
#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <objc/runtime.h>

#import <whist/logging/logging.h>

/*
============================
Public Function Implementations
============================
*/

int display_notification(char *title, char *msg) {
    NSUserNotification *n = [[NSUserNotification alloc] init];

    LOG_INFO("Trying to display notif on OSX: %s | %s", title, msg);

    n.title = [NSString stringWithUTF8String:title];
    n.informativeText = [NSString stringWithUTF8String:msg];

    [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:n];

    return 0;
}
