#include "native.h"
#include <SDL2/SDL_syswm.h>
#include <Cocoa/Cocoa.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <VideoToolbox/VideoToolbox.h>

void sdl_native_hide_taskbar(void) {
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

int sdl_native_set_titlebar_color(SDL_Window *window, const WhistRGBColor *color) {
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

    if (color_requires_dark_text(*color)) {
        // dark font color for titlebar
        [native_window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantLight]];
    } else {
        // light font color for titlebar
        [native_window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }

    // RGBA; in this case, alpha is just 1.0.
    const CGFloat components[4] = {(float)color->red / 255., (float)color->green / 255.,
                                   (float)color->blue / 255., 1.0};

    // Use color space for current monitor for perfect color matching
    [native_window
        setBackgroundColor:[NSColor colorWithColorSpace:[[native_window screen] colorSpace]
                                             components:&components[0]
                                                  count:4]];
    return 0;
}

int sdl_native_get_dpi(SDL_Window *window) {
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

void sdl_native_init_window_options(SDL_Window *window) {
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

// These can be global static, not per-frontend, since screensaver is global.
// There may be race conditions with mutex creation, though.
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

void sdl_native_declare_user_activity(void) {
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

/*
typedef struct FileDragState {
    id left_mouse_drag_listener;
    id left_mouse_up_listener;
    id swipe_listener;
    bool mouse_down;
    int change_count;
    bool active;
    WhistTimer sent_drag_move_event_timer;
    int position_x;
    int position_y;
} FileDragState;

static bool mouse_in_window(WhistFrontend *frontend) {
    SDLFrontendContext *context = frontend->context;
    FileDragState *state = context->file_drag_data;

    // If window is minimized or not visible, then the mouse is definitely not in window
    //     TODO: return false for drags over occluded parts of an unoccluded window
    if (SDL_GetWindowFlags(context->window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED)) {
        return false;
    }

    int window_x, window_y, mouse_x, mouse_y;
    int window_w, window_h;
    SDL_GetWindowPosition(context->window, &window_x, &window_y);
    SDL_GetWindowSize(context->window, &window_w, &window_h);
    // Mouse is not active in window - so we must use the global mouse and manually transform
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);

    state->position_x = mouse_x - window_x;
    state->position_y = mouse_y - window_y;

    // If the mouse is in the window
    if (mouse_x >= window_x && mouse_x <= window_x + window_w && mouse_y >= window_y &&
        mouse_y <= window_y + window_h) {
        return true;
    }

    return false;
}

static void push_drag_end_event(WhistFrontend *frontend) {
    SDLFrontendContext *context = frontend->context;
    FileDragState *state = context->file_drag_data;
    state->active = false;

    SDL_Event event = {0};
    event.type = context->file_drag_event_id;
    FrontendFileDragEvent *drag_event = safe_malloc(sizeof(FrontendFileDragEvent));
    memset(drag_event, 0, sizeof(FrontendFileDragEvent));
    drag_event->end_drag = true;
    drag_event->group_id = state->change_count;  // use change count as group ID
    event.user.data1 = drag_event;
    SDL_PushEvent(&event);
}

static void push_drag_start_event(WhistFrontend *frontend, char *filename) {
    SDLFrontendContext *context = frontend->context;
    FileDragState *state = context->file_drag_data;

    SDL_Event event = {0};
    event.type = context->file_drag_event_id;
    FrontendFileDragEvent *drag_event = safe_malloc(sizeof(FrontendFileDragEvent));
    memset(drag_event, 0, sizeof(FrontendFileDragEvent));
    drag_event->end_drag = false;
    drag_event->filename = strdup(filename);
    drag_event->group_id = state->change_count;  // use change count as group ID
    if (!drag_event->filename) {
        // strdup failed
        return;
    }
    event.user.data1 = drag_event;
    SDL_PushEvent(&event);

    start_timer(&state->sent_drag_move_event_timer);
}

static void push_drag_event(WhistFrontend *frontend) {
    SDLFrontendContext *context = frontend->context;
    FileDragState *state = context->file_drag_data;

    // Send a drag event at most every 15 ms
    if (state->active && get_timer(&state->sent_drag_move_event_timer) * MS_IN_SECOND < 15) {
        return;
    }
    start_timer(&state->sent_drag_move_event_timer);

    state->active = true;

    SDL_Event event = {0};
    event.type = context->file_drag_event_id;
    FrontendFileDragEvent *drag_event = safe_malloc(sizeof(FrontendFileDragEvent));
    memset(drag_event, 0, sizeof(FrontendFileDragEvent));
    drag_event->end_drag = false;
    drag_event->position.x = state->position_x;
    drag_event->position.y = state->position_y;
    drag_event->group_id = state->change_count;  // use change count as group ID
    event.user.data1 = drag_event;
    SDL_PushEvent(&event);
}

void sdl_native_init_external_drag_handler(WhistFrontend *frontend) {
    //    Initializes global event handlers for left mouse down dragged
    //    and left mouse up events. The drag board change count changes
    //    when a new file is being dragged. However, it is tricky to detect
    //    continual drags of files because a user can drag something that
    //    is not a file (like a highlight for example) and the drag board
    //    change count will stay the same. Thus we need a way to determine if
    //    the current drag event corresponds to a new, picked file. We do this
    //    by monitoring when the mouse up event occurs which signals that the user
    //    is finished dragging the file in question.
    SDLFrontendContext *context = frontend->context;
    context->file_drag_data = safe_malloc(sizeof(FileDragState));
    FileDragState *state = context->file_drag_data;
    memset(state, 0, sizeof(FileDragState));

    // Initialize change count since global drag board change count starts incrementing at system
    // start
    NSPasteboard *change_count_pb = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
    state->change_count = (int)[change_count_pb changeCount];
    state->mouse_down = false;
    state->active = false;

    // Set event handler for detecting when a new file is being dragged
    state->left_mouse_drag_listener = [NSEvent
        addGlobalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDragged
                                      handler:^(NSEvent *event) {
                                        NSPasteboard *pb =
                                            [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
                                        int change_count = (int)[pb changeCount];
                                        if (mouse_in_window(frontend)) {
                                            if (change_count > state->change_count) {
                                                // If a new file has been loaded, mark as mouse
                                                // down and update changecount
                                                state->change_count = change_count;
                                                state->mouse_down = true;

                                                if ([[pb types]
                                                        containsObject:NSFilenamesPboardType]) {
                                                    NSArray *files = [pb
                                                        propertyListForType:NSFilenamesPboardType];
                                                    for (NSString *file in files) {
                                                        push_drag_start_event(
                                                            frontend,
                                                            (char *)[[file lastPathComponent]
                                                                UTF8String]);
                                                    }
                                                }
                                            } else if (state->mouse_down) {
                                                // We are continuing to drag our file from its
                                                // original mousedown selection
                                                push_drag_event(frontend);
                                            }
                                        } else if (state->active) {
                                            push_drag_end_event(frontend);
                                        }
                                      }];

    // Mouse up event indicates that file is no longer being dragged
    state->left_mouse_up_listener =
        [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskLeftMouseUp
                                               handler:^(NSEvent *event) {
                                                 state->mouse_down = false;
                                                 push_drag_end_event(frontend);
                                               }];

    // Swipe gesture event indicates that file is no longer being dragged
    state->swipe_listener = [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskSwipe
                                                                   handler:^(NSEvent *event) {
                                                                     state->mouse_down = false;
                                                                     push_drag_end_event(frontend);
                                                                   }];
}

void sdl_native_destroy_external_drag_handler(WhistFrontend *frontend) {
    //    NSEvent event listeners are removed by passing in their ids
    //    to the removeMonitor call
    SDLFrontendContext *context = frontend->context;
    FileDragState *state = context->file_drag_data;

    if (state == NULL) {
        return;
    }

    if (state->left_mouse_drag_listener != NULL) {
        [NSEvent removeMonitor:state->left_mouse_drag_listener];
    }

    if (state->left_mouse_up_listener != NULL) {
        [NSEvent removeMonitor:state->left_mouse_up_listener];
    }

    if (state->swipe_listener != NULL) {
        [NSEvent removeMonitor:state->swipe_listener];
    }

    context->file_drag_data = NULL;
    free(state);
}
*/
