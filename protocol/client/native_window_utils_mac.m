/**
 * Copyright Fractal Computers, Inc. 2021
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
#include <fractal/core/fractal.h>
#include "sdl_utils.h"

/*
============================
Public Function Implementations
============================
*/

void hide_native_window_taskbar() {
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

NSWindow *get_native_window(SDL_Window *window) {
    SDL_SysWMinfo sys_info = {0};
    SDL_GetWindowWMInfo(window, &sys_info);
    return sys_info.info.cocoa.window;
}

int set_native_window_color(SDL_Window *window, FractalRGBColor color) {
    /*
        Set the color of the titlebar of the native macOS window, and the corresponding
        titlebar text color.

        Arguments:
            window (SDL_Window*):       SDL window wrapper for the NSWindow whose titlebar to modify
            color (FractalRGBColor):    The RGB color to use when setting the titlebar color

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

    // We technically only need to set this the first time
    [native_window setTitlebarAppearsTransparent:true];

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

FractalYUVColor get_frame_color(uint8_t *y_data, uint8_t *u_data, uint8_t *v_data,
                                bool using_hardware) {
    FractalYUVColor yuv_color = {0};
    if (using_hardware) {
        if (y_data) {
            CVPixelBufferRef frame_data = (CVPixelBufferRef)y_data;
            CVPixelBufferLockBaseAddress(frame_data, kCVPixelBufferLock_ReadOnly);
            uint8_t *luma_base_address =
                (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(frame_data, 0);
            uint8_t *chroma_base_address =
                (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(frame_data, 1);
            yuv_color.y = luma_base_address[0];
            yuv_color.u = chroma_base_address[0];
            yuv_color.v = chroma_base_address[1];
            CVPixelBufferUnlockBaseAddress(frame_data, kCVPixelBufferLock_ReadOnly);
        }
    } else {
        if (y_data && u_data && v_data) {
            yuv_color.y = y_data[0];
            yuv_color.u = u_data[0];
            yuv_color.v = v_data[0];
        }
    }
    return yuv_color;
}

static IOPMAssertionID power_assertion_id = kIOPMNullAssertionID;
static SDL_mutex *last_user_activity_timer_mutex = NULL;
static clock last_user_activity_timer;
static bool assertion_set = false;

// Timeout the screensaver after 1min of inactivity,
// Their own screensaver timer will pick up the rest of the time
#define SCREENSAVER_TIMEOUT_SECONDS (1 * 60)

int user_activity_deactivator(void *unique) {
    UNUSED(unique);

    while (true) {
        // Only wake up once every 30 seconds, resolution doesn't matter that much here
        fractal_sleep(30 * 1000);
        safe_SDL_LockMutex(last_user_activity_timer_mutex);
        if (get_timer(last_user_activity_timer) > SCREENSAVER_TIMEOUT_SECONDS && assertion_set) {
            IOReturn result = IOPMAssertionRelease(power_assertion_id);
            if (result != kIOReturnSuccess) {
                LOG_ERROR("IOPMAssertionRelease Failed!");
            }
            assertion_set = false;
        }
        safe_SDL_UnlockMutex(last_user_activity_timer_mutex);
    }

    return 0;
}

void declare_user_activity() {
    if (!last_user_activity_timer_mutex) {
        last_user_activity_timer_mutex = safe_SDL_CreateMutex();
        start_timer(&last_user_activity_timer);
        fractal_create_thread(user_activity_deactivator, "user_activity_deactivator", NULL);
    }

    // Static bool because we'll only accept one failure,
    // in order to not spam LOG_ERROR's
    static bool failed = false;
    if (!failed) {
        // Reset the last activity timer
        safe_SDL_LockMutex(last_user_activity_timer_mutex);
        start_timer(&last_user_activity_timer);
        safe_SDL_UnlockMutex(last_user_activity_timer_mutex);

        // Declare user activity to MacOS
        if (!assertion_set) {
            IOReturn result =
                IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
                                            CFSTR("FractalNewFrameActivity"), &power_assertion_id);
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
