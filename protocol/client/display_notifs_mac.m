#import "display_notifs.h"

#import <stdio.h>
#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <objc/runtime.h>

#import <whist/logging/logging.h>

NSString *fakeBundleIdentifier = nil;
bool bundle_spoofed = false;

@implementation NSBundle(swizle)

- (NSString *)__bundleIdentifier
{
    if (self == [NSBundle mainBundle]) {
        return fakeBundleIdentifier ? fakeBundleIdentifier : @"com.whisttechnologies.whist";
    } else {
        return [self __bundleIdentifier];
    }
}

@end

BOOL installNSBundleHook()
{
    Class class = objc_getClass("NSBundle");
    if (class) {
        method_exchangeImplementations(class_getInstanceMethod(class, @selector(bundleIdentifier)),
                                       class_getInstanceMethod(class, @selector(__bundleIdentifier)));
        return YES;
    }
    return NO;
}

@interface AppDelegate : NSObject <NSUserNotificationCenterDelegate>
@end

@implementation AppDelegate

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center 
                               shouldPresentNotification:(NSUserNotification *)notification {
  return YES;
}

@end

bool notif_bundle_initialized() {
  return bundle_spoofed;
}

int init_notif_bundle() {
    if (installNSBundleHook()) {
        bundle_spoofed = true;
        return 0;
    }
  return -1;
}

int deliver_notification(char *title, char *msg) {
    NSUserNotification *n = [[NSUserNotification alloc] init];

    n.title = [NSString stringWithUTF8String:title];
    n.informativeText = [NSString stringWithUTF8String:msg];

    [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:n];

    return 0;
}
