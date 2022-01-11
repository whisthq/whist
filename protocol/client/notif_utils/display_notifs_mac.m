/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file display_notifs_mac.m
 * @brief Handles client-side notification display on MacOS

Interfaces with native libraries to display notifications thrown from the server application.

*/

/*
============================
Includes
============================
*/

#import "display_notifs.h"

#import <stdio.h>
#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <objc/runtime.h>

#import <whist/logging/logging.h>

/*
============================
Public Functions
============================
*/

int native_show_notification(char *title, char *msg);

/*
============================
Public Function Implementations
============================
*/

int native_show_notification(char *title, char *msg) {
    NSUserNotification *n = [[NSUserNotification alloc] init];

    n.title = [NSString stringWithUTF8String:title];
    n.informativeText = [NSString stringWithUTF8String:msg];

    [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:n];

    return 0;
}
