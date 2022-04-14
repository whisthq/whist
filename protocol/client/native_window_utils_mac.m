/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file native_window_utils_mac.m
 * @brief This file implements macOS APIs for native window
 *        modifications not handled by SDL.
============================
Usage
============================

Use these functions to modify the appearance or behavior of the native window
underlying the SDL window. These functions will almost always need to be called
from the same thread as SDL window creation and SDL event handling; usually this
is the main thread.
*/

/*
============================
Includes
============================
*/

#include "native_window_utils.h"
#include <SDL2/SDL_syswm.h>
#include <Cocoa/Cocoa.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <VideoToolbox/VideoToolbox.h>
#include <whist/core/whist.h>
#include "sdl_utils.h"

/*
============================
Public Function Implementations
============================
*/

static id global_left_mouse_drag_listener;
static id global_left_mouse_up_listener;
static id global_swipe_listener;

void initialize_out_of_window_drag_handlers(WhistFrontend *frontend) {
    /*
        Initializes global event handlers for left mouse down dragged
        and left mouse up events. The drag board change count changes
        when a new file is being dragged. However, it is tricky to detect
        continual drags of files because a user can drag something that
        is not a file (like a highlight for example) and the drag board
        change count will stay the same. Thus we need a way to determine if
        the current drag event corresponds to a new, picked file. We do this
        by monitoring when the mouse up event occurs which signals that the user
        is finished dragging the file in question.
    */

    // Monitor change count and mouse press as state variables
    global_left_mouse_drag_listener = NULL;
    global_left_mouse_up_listener = NULL;
    global_swipe_listener = NULL;
    static bool file_drag_mouse_down;
    static int current_change_count;

    static bool file_drag_began_and_ongoing = false;

    // Initialize change count since global drag board change count starts incrementing at system
    // start
    NSPasteboard *change_count_pb = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
    current_change_count = (int)[change_count_pb changeCount];
    file_drag_mouse_down = false;

    // Set event handler for detecting when a new file is being dragged
    global_left_mouse_drag_listener = [NSEvent
        addGlobalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDragged
                                      handler:^(NSEvent *event) {
                                        NSPasteboard *pb =
                                            [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
                                        int change_count = (int)[pb changeCount];
                                        if (change_count > current_change_count) {
                                            // If a new file has been loaded, mark as mouse
                                            // down and update changecount
                                            current_change_count = change_count;
                                            file_drag_mouse_down = true;

                                            // if ( [[pb types] containsObject:NSFilenamesPboardType] ) {
                                            //     NSArray *files = [pb propertyListForType:NSFilenamesPboardType];
                                            //     int numberOfFiles = [files count];
                                            //     // Perform operation using the list of files
                                            //     // LOG_INFO("drag content: %s", (char*)[[pb dataForType:NSPasteboardTypeFileURL] bytes]);
                                            //     LOG_INFO("dragging %d files", numberOfFiles);
                                            //     LOG_INFO("first file: %s", (char*)[[files objectAtIndex:0] UTF8String]);
                                            // }

                                            // NSArray *files = [pb propertyListForType:NSFilenamesPboardType];

                                            // // TODO: turn *files into a \n-separated string of filenames (without paths)
                                            // sdl_handle_begin_file_drag(files);

                                        } else if (file_drag_mouse_down) {
                                            // We are continuing to drag our file from its
                                            // original mousedown selection - turn over to sdl
                                            // handler

                                            sdl_handle_drag_event(frontend);
                                        }
                                      }];

    // Mouse up event indicates that file is no longer being dragged
    global_left_mouse_up_listener =
        [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskLeftMouseUp
                                               handler:^(NSEvent *event) {
                                                 file_drag_mouse_down = false;
                                                 sdl_end_drag_event();
                                               }];

    // Swipe gesture event indicates that file is no longer being dragged
    global_swipe_listener = [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskSwipe
                                                                   handler:^(NSEvent *event) {
                                                                     file_drag_mouse_down = false;
                                                                     sdl_end_drag_event();
                                                                   }];
}

// TODO: Destroy these event handlers per WhistFrontend, similar to initialization.
void destroy_out_of_window_drag_handlers(void) {
    /*
        NSEvent event listeners are removed by passing in their ids
        to the removeMonitor call
    */

    if (global_left_mouse_drag_listener != NULL) {
        [NSEvent removeMonitor:global_left_mouse_drag_listener];
    }

    if (global_left_mouse_drag_listener != NULL) {
        [NSEvent removeMonitor:global_left_mouse_up_listener];
    }

    if (global_swipe_listener != NULL) {
        [NSEvent removeMonitor:global_swipe_listener];
    }
}

void hide_native_window_taskbar(void) {
    /*
        Hide the taskbar icon for the app. This only works on macOS (for Window and Linux,
        SDL already implements this in the `SDL_WINDOW_SKIP_TASKBAR` flag).
    */
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    // most likely not necessary
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];

    // hack to make menubar show up
    [NSMenu setMenuBarVisible:NO];
    [NSMenu setMenuBarVisible:YES];
}

static NSWindow *get_native_window(SDL_Window *window) {
    SDL_SysWMinfo sys_info = {0};
    SDL_GetWindowWMInfo(window, &sys_info);
    return sys_info.info.cocoa.window;
}

void init_native_window_options(SDL_Window *window) {
    /*
        Initialize the customized native window. This is called from the main thread
        right after the window finishes loading.

        Arguments:
            window (SDL_Window*): The SDL window wrapper for the NSWindow to customize.
    */
    NSWindow *native_window = get_native_window(window);

    // Hide the titlebar
    [native_window setTitlebarAppearsTransparent:true];

    // Hide the title itself
    [native_window setTitleVisibility:NSWindowTitleHidden];
}

int set_native_window_color(SDL_Window *window, WhistRGBColor color) {
    /*
        Set the color of the titlebar of the native macOS window, and the corresponding
        titlebar text color.

        Arguments:
            window (SDL_Window*):       SDL window wrapper for the NSWindow whose titlebar to modify
            color (WhistRGBColor):    The RGB color to use when setting the titlebar color

        Returns:
            (int): 0 on success, -1 on failure.
    */

    NSWindow *native_window = get_native_window(window);

    if (color_requires_dark_text(color)) {
        // dark font color for titlebar
        [native_window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantLight]];
    } else {
        // light font color for titlebar
        [native_window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }

    // RGBA; in this case, alpha is just 1.0.
    const CGFloat components[4] = {(float)color.red / 255., (float)color.green / 255.,
                                   (float)color.blue / 255., 1.0};

    // Use color space for current monitor for perfect color matching
    [native_window
        setBackgroundColor:[NSColor colorWithColorSpace:[[native_window screen] colorSpace]
                                             components:&components[0]
                                                  count:4]];
    return 0;
}

int get_native_window_dpi(SDL_Window *window) {
    /*
        Get the DPI for the display of the provided window.

        Arguments:
            window (SDL_Window*):       SDL window whose DPI to retrieve.

        Returns:
            (int): The DPI as an int, with 96 as a base.
    */

    NSWindow *native_window = get_native_window(window);

    const CGFloat scale_factor = [[native_window screen] backingScaleFactor];
    return (int)(96 * scale_factor);
}

static IOPMAssertionID power_assertion_id = kIOPMNullAssertionID;
static WhistMutex last_user_activity_timer_mutex;
static WhistTimer last_user_activity_timer;
static bool assertion_set = false;

// Timeout the screensaver after 1min of inactivity,
// Their own screensaver timer will pick up the rest of the time
#define SCREENSAVER_TIMEOUT_SECONDS (1 * 60)

static int user_activity_deactivator(void *unique) {
    UNUSED(unique);

    while (true) {
        // Only wake up once every 30 seconds, resolution doesn't matter that much here
        whist_sleep(30 * MS_IN_SECOND);
        whist_lock_mutex(last_user_activity_timer_mutex);
        if (get_timer(&last_user_activity_timer) > SCREENSAVER_TIMEOUT_SECONDS && assertion_set) {
            IOReturn result = IOPMAssertionRelease(power_assertion_id);
            if (result != kIOReturnSuccess) {
                LOG_ERROR("IOPMAssertionRelease Failed!");
            }
            assertion_set = false;
        }
        whist_unlock_mutex(last_user_activity_timer_mutex);
    }

    return 0;
}

void declare_user_activity(void) {
    if (!last_user_activity_timer_mutex) {
        last_user_activity_timer_mutex = whist_create_mutex();
        start_timer(&last_user_activity_timer);
        whist_create_thread(user_activity_deactivator, "user_activity_deactivator", NULL);
    }

    // Static bool because we'll only accept one failure,
    // in order to not spam LOG_ERROR's
    static bool failed = false;
    if (!failed) {
        // Reset the last activity timer
        whist_lock_mutex(last_user_activity_timer_mutex);
        start_timer(&last_user_activity_timer);
        whist_unlock_mutex(last_user_activity_timer_mutex);

        // Declare user activity to MacOS
        if (!assertion_set) {
            IOReturn result =
                IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
                                            CFSTR("WhistNewFrameActivity"), &power_assertion_id);
            // Immediatelly calling IOPMAssertionRelease here,
            // Doesn't seem to fully reset the screensaver timer
            // Instead, we need to call IOPMAssertionRelease a minute later
            if (result == kIOReturnSuccess) {
                // If the call succeeded, mark as set
                assertion_set = true;
            } else {
                LOG_ERROR("Failed to IOPMAssertionCreateWithName");
                // If the call failed, we'll just disable the screensaver then
                SDL_DisableScreenSaver();
                failed = true;
            }
        }
    }
}
