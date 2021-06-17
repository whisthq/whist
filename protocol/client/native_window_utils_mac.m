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
