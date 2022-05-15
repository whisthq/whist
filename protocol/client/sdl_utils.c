/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sdl_utils.c
 * @brief This file contains the code to create and destroy SDL windows on the
 *        client.
============================
Usage
============================

initSDL gets called first to create an SDL window, and destroySDL at the end to
close the window.
*/

/*
============================
Includes
============================
*/

#include "sdl_utils.h"
#include <whist/core/whist_string.h>
#include <whist/utils/png.h>
#include <whist/utils/lodepng.h>
#include "frontend/frontend.h"
#include <whist/logging/log_statistic.h>
#include <whist/utils/command_line.h>
#include <whist/network/network_algorithm.h>

#include <whist/utils/color.h>
#include "client_utils.h"
#include "whist/core/whist.h"

static WhistMutex frontend_render_mutex;

// pending render Update
static volatile bool pending_render = false;

// Video frame update.
static AVFrame* pending_video_frame;

// for cursor update. The value is writen by the video render thread, and taken away by the main
// thread.
static WhistCursorInfo* pending_cursor_info = NULL;
// the mutex to protect pending_cursor_info
static WhistMutex pending_cursor_info_mutex;

// The background color for the loading screen
static const WhistRGBColor background_color = {17, 24, 39};  // #111827 (thanks copilot)

// Frontend instance used for delivering cross-thread events.
static WhistFrontend* event_frontend;

static const char* frontend_type;
COMMAND_LINE_STRING_OPTION(frontend_type, 'f', "frontend", WHIST_ARGS_MAXLEN,
                           "Which frontend type to attempt to use.  Default: sdl.")

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Updates the framebuffer from any pending framebuffer call,
 *                                 Then RenderPresent if sdl_render_framebuffer is pending.
 */
static void sdl_present_pending_framebuffer(WhistFrontend* frontend);

/**
 * @brief                          Renders out the insufficient bandwidth error message
 *
 * @note                           Must be called on the main thread
 */
static void render_insufficient_bandwidth(WhistFrontend* frontend);

/*
============================
Public Function Implementations
============================
*/

WhistFrontend* create_frontend(void) {
    WhistFrontend* frontend = whist_frontend_create(frontend_type ? frontend_type : "sdl");
    if (frontend == NULL) {
        LOG_ERROR("Failed to create frontend");
        return NULL;
    }

    // TODO: Stop passing width/height/title here.
    if (whist_frontend_init(frontend, 0, 0, "Whist", &background_color) != WHIST_SUCCESS) {
        whist_frontend_destroy(frontend);
        LOG_ERROR("Failed to initialize frontend");
        return NULL;
    }

    pending_cursor_info_mutex = whist_create_mutex();
    frontend_render_mutex = whist_create_mutex();
    // set pending_cursor_info to NULL for safety, not necessary at the moment since
    // create_frontend() is only called once.
    pending_cursor_info = NULL;

    pending_video_frame = NULL;
    pending_render = false;

    // After creating the window, we will grab DPI-adjusted dimensions in real
    // pixels
    int w, h;
    whist_frontend_get_window_pixel_size(frontend, 0, &w, &h);
    network_algo_set_dimensions(w, h);

    event_frontend = frontend;

    return frontend;
}

void destroy_frontend(WhistFrontend* frontend) {
    av_frame_free(&pending_video_frame);

    LOG_INFO("Destroying SDL");

    whist_destroy_mutex(pending_cursor_info_mutex);
    whist_destroy_mutex(frontend_render_mutex);

    if (frontend) {
        whist_frontend_destroy(frontend);
    }
}

WhistMutex window_resize_mutex;
WhistTimer window_resize_timer;
// pending_resize_message should be set to true if sdl event handler was not able to process resize
// event due to throttling, so the main loop should process it
volatile bool pending_resize_message = false;
void sdl_renderer_resize_window(WhistFrontend* frontend, int width, int height) {
    // Try to make pixel width and height conform to certain desirable dimensions
    int current_width, current_height;
    whist_frontend_get_window_pixel_size(frontend, 0, &current_width, &current_height);

    LOG_INFO("Received resize event for %dx%d, currently %dx%d", width, height, current_width,
             current_height);
    //
    int dpi = whist_frontend_get_window_dpi(frontend);
    // If we're on linux, our desired and current should match up
    int desired_width = current_width;
    int desired_height = current_height;

#if !OS_IS(OS_LINUX)

    // The server will round the dimensions up in order to satisfy the YUV pixel format
    // requirements. Specifically, it will round the width up to a multiple of 8 and the height up
    // to a multiple of 2. Here, we try to force the window size to be valid values so the
    // dimensions of the client and server match. We round down rather than up to avoid extending
    // past the size of the display.
    desired_width = current_width - (current_width % 8);
    desired_height = current_height - (current_height % 2);
#endif  // non-Linux

    // Regardless of OS, constrain to min width/height
    desired_width = max(MIN_SCREEN_WIDTH, desired_width);
    desired_height = max(MIN_SCREEN_HEIGHT, desired_height);
    static int prev_desired_width = 0;
    static int prev_desired_height = 0;
    static int tries =
        0;  // number of attempts to force window size to be prev_desired_width/height
    if (current_width != desired_width || current_height != desired_height) {
        // Avoid trying to force the window size forever, stop after 4 attempts
        if (!(prev_desired_width == desired_width && prev_desired_height == desired_height &&
              tries > 4)) {
            if (prev_desired_width == desired_width && prev_desired_height == desired_height) {
                tries++;
            } else {
                prev_desired_width = desired_width;
                prev_desired_height = desired_height;
                tries = 0;
            }

            // The default DPI (no scaling) is 96, hence this magic number to divide by the scaling
            // factor
            whist_frontend_resize_window(frontend, 0, desired_width * 96 / dpi,
                                         desired_height * 96 / dpi);
            LOG_INFO("Forcing a resize from %dx%d to %dx%d", current_width, current_height,
                     desired_width, desired_height);
            whist_frontend_get_window_pixel_size(frontend, 0, &current_width, &current_height);

            if (current_width != desired_width || current_height != desired_height) {
                LOG_WARNING("Failed to force resize -- got %dx%d instead of desired %dx%d",
                            current_width, current_height, desired_width, desired_height);
            }
        }
    }

    network_algo_set_dimensions(current_width, current_height);

    whist_lock_mutex(window_resize_mutex);
    pending_resize_message = true;
    whist_unlock_mutex(window_resize_mutex);

    LOG_INFO("Window resized to %dx%d (Actual %dx%d)", width, height, current_width,
             current_height);
}

void sdl_update_framebuffer(AVFrame* frame) {
    whist_lock_mutex(frontend_render_mutex);

    // Check dimensions as a fail-safe
    if ((frame->width < 0 || frame->width > MAX_SCREEN_WIDTH) ||
        (frame->height < 0 || frame->height > MAX_SCREEN_HEIGHT)) {
        LOG_ERROR("Invalid Dimensions! %dx%d. nv12 update dropped", frame->width, frame->height);
    } else {
        if (pending_video_frame) {
            // Free a previous undisplayed frame.
            av_frame_free(&pending_video_frame);
        }
        pending_video_frame = frame;
    }

    whist_unlock_mutex(frontend_render_mutex);

    whist_frontend_interrupt(event_frontend);
}

void sdl_render_framebuffer(void) {
    whist_lock_mutex(frontend_render_mutex);
    // Mark a render as pending
    pending_render = true;
    whist_unlock_mutex(frontend_render_mutex);
}

bool sdl_render_pending(void) {
    whist_lock_mutex(frontend_render_mutex);
    bool pending_render_val = pending_render;
    whist_unlock_mutex(frontend_render_mutex);
    return pending_render_val;
}

void sdl_set_cursor_info_as_pending(const WhistCursorInfo* cursor_info) {
    // do the operations with a local pointer first, so to minimize locking
    WhistCursorInfo* temp_cursor_info = safe_malloc(whist_cursor_info_get_size(cursor_info));
    memcpy(temp_cursor_info, cursor_info, whist_cursor_info_get_size(cursor_info));

    whist_lock_mutex(pending_cursor_info_mutex);
    if (pending_cursor_info != NULL) {
        // if pending_cursor_info is not null, it means an old cursor hasn't been rendered yet.
        // simply free the old one and overwrite the pending_cursor_info.
        // The duty of free a not-yet-rendered cursor is at producer side, since the ownership
        // hasn't been taken away.
        free(pending_cursor_info);
    }

    // assign the local pointer to the pending_cursor_info
    pending_cursor_info = temp_cursor_info;
    whist_unlock_mutex(pending_cursor_info_mutex);
}

void sdl_present_pending_cursor(WhistFrontend* frontend) {
    WhistTimer statistics_timer;
    WhistCursorInfo* temp_cursor_info = NULL;

    // if there is a pending curor, take the ownership of pending_cursor_info, and assign it to the
    // local pointer. do rendering with the local pointer after unlock, to minimize locking.
    whist_lock_mutex(pending_cursor_info_mutex);
    if (pending_cursor_info) {
        temp_cursor_info = pending_cursor_info;
        pending_cursor_info = NULL;
    }
    whist_unlock_mutex(pending_cursor_info_mutex);

    if (temp_cursor_info != NULL) {
        TIME_RUN(whist_frontend_set_cursor(frontend, temp_cursor_info), VIDEO_CURSOR_UPDATE_TIME,
                 statistics_timer);
        // Cursors need not be double-rendered, so we just unset the cursor image here.
        // The duty of freeing a rendered cursor is at consumer side (here), since the ownership is
        // taken.
        free(temp_cursor_info);
    }
}

void sdl_render_window_titlebar_color(int id, WhistRGBColor color) {
    /*
      Update window titlebar color using the colors of the new frame
     */
    static WhistRGBColor current_color = {0, 0, 0};
    if (memcmp(&current_color, &color, sizeof(color))) {
        WhistRGBColor* new_color = safe_malloc(sizeof(color));
        *new_color = color;
        // whist_frontend_set_titlebar_color is now responsible for free'ing new_color
        whist_frontend_set_titlebar_color(event_frontend, id, new_color);
        current_color = color;
    }
}

void sdl_set_window_title(int id, const char* requested_window_title) {
    size_t len = strlen(requested_window_title) + 1;
    char* new_window_title = safe_malloc(len);
    safe_strncpy(new_window_title, requested_window_title, len);

    // whist_frontend_set_title is now responsible for free'ing new_window_title
    whist_frontend_set_title(event_frontend, id, new_window_title);
}

void sdl_set_fullscreen(int id, bool is_fullscreen) {
    whist_frontend_set_window_fullscreen(event_frontend, id, is_fullscreen);
}

void sdl_client_display_notification(const WhistNotification* notif) {
    whist_frontend_display_notification(event_frontend, notif);
}

void sdl_update_pending_tasks(WhistFrontend* frontend) {
    // Check if a pending window resize message should be sent to server
    whist_lock_mutex(window_resize_mutex);
    if (pending_resize_message &&
        get_timer(&window_resize_timer) >= WINDOW_RESIZE_MESSAGE_INTERVAL / (float)MS_IN_SECOND) {
        pending_resize_message = false;
        send_message_dimensions(frontend);
        start_timer(&window_resize_timer);
    }
    whist_unlock_mutex(window_resize_mutex);

    sdl_present_pending_cursor(frontend);

    sdl_present_pending_framebuffer(frontend);
}

void sdl_utils_check_private_vars(bool* pending_resize_message_ptr) {
    /*
      This function sets the variables pointed to by each of the non-NULL parameters (with the
      exception of native_window_color_is_null_ptr, which has a slightly different purpose) with the
      values held by the corresponding sdl_utils.c globals. If native_window_color_is_null_ptr is
      not NULL, we set the value pointed to by it with a boolean indicating whether the
      native_window_color global pointer is NULL.
     */

    if (pending_resize_message_ptr) {
        if (window_resize_mutex) {
            whist_lock_mutex(window_resize_mutex);
            *pending_resize_message_ptr = pending_resize_message;
            whist_unlock_mutex(window_resize_mutex);
        } else {
            *pending_resize_message_ptr = pending_resize_message;
        }
    }
}

/*
============================
Private Function Implementations
============================
*/

static void sdl_present_pending_framebuffer(WhistFrontend* frontend) {
    static bool insufficient_bandwidth = false;

    // Render out the current framebuffer, if there's a pending render
    whist_lock_mutex(frontend_render_mutex);

    // Render the error message immediately during state transition to insufficient bandwidth
    if (network_algo_is_insufficient_bandwidth()) {
        if (insufficient_bandwidth == false) {
            pending_render = true;
        }
        insufficient_bandwidth = true;
    } else {
        insufficient_bandwidth = false;
    }

    // If there's no pending render or overlay visualizations, just do nothing,
    // Don't consume and discard any pending nv12 or loading screen.
    if (!pending_render) {
        whist_unlock_mutex(frontend_render_mutex);
        return;
    }

    // Wipes the renderer to background color before we present
    whist_gpu_lock();
    whist_frontend_paint_solid(frontend, 0, &background_color);
    whist_gpu_unlock();

    WhistTimer statistics_timer;
    start_timer(&statistics_timer);

    // If there is a new video frame then update the frontend texture
    // with it.
    if (pending_video_frame) {
        whist_gpu_lock();
        whist_frontend_update_video(frontend, pending_video_frame);
        whist_gpu_unlock();

        // If the frontend needs to take a reference to the frame data
        // then it has done so, so we can free this frame immediately.
        av_frame_free(&pending_video_frame);
    }

    whist_gpu_lock();
    whist_frontend_paint_video(frontend);
    whist_gpu_unlock();

    if (insufficient_bandwidth) {
        render_insufficient_bandwidth(frontend);
    }

    log_double_statistic(VIDEO_RENDER_TIME, get_timer(&statistics_timer) * MS_IN_SECOND);
    whist_unlock_mutex(frontend_render_mutex);

    // RenderPresent outside of the mutex, since RenderCopy made a copy anyway
    // and this will take ~8ms if VSYNC is on.
    // (If this causes a bug, feel free to pull back to inside of the mutex)
    whist_gpu_lock();
    TIME_RUN(whist_frontend_render(frontend), VIDEO_RENDER_TIME, statistics_timer);
    whist_gpu_unlock();

    whist_lock_mutex(frontend_render_mutex);
    pending_render = false;
    whist_unlock_mutex(frontend_render_mutex);
}

EMBEDDED_OBJECT(insufficient_bandwidth_500x50);
EMBEDDED_OBJECT(insufficient_bandwidth_750x75);
EMBEDDED_OBJECT(insufficient_bandwidth_1000x100);
EMBEDDED_OBJECT(insufficient_bandwidth_1500x150);

static void render_insufficient_bandwidth(WhistFrontend* frontend) {
    // List of images.  Width must be in ascending order!
    static const struct {
        int width;
        const uint8_t* data;
        const size_t* size;
    } images[] = {
        {
            500,
            insufficient_bandwidth_500x50,
            &insufficient_bandwidth_500x50_size,
        },
        {
            750,
            insufficient_bandwidth_750x75,
            &insufficient_bandwidth_750x75_size,
        },
        {
            1000,
            insufficient_bandwidth_1000x100,
            &insufficient_bandwidth_1000x100_size,
        },
        {
            1500,
            insufficient_bandwidth_1500x150,
            &insufficient_bandwidth_1500x150_size,
        },
    };
#define TARGET_WIDTH_IN_INCHES 5.0
    int chosen_idx = 0;
    int dpi = whist_frontend_get_window_dpi(frontend);
    // Choose an image closest to TARGET_WIDTH_IN_INCHES.
    // And it should be at least TARGET_WIDTH_IN_INCHES.
    for (int i = 0; i < (int)ARRAY_LENGTH(images); i++) {
        double width_in_inches = (double)images[i].width / dpi;
        if (width_in_inches > TARGET_WIDTH_IN_INCHES) {
            chosen_idx = i;
            break;
        }
    }

    whist_frontend_paint_png(frontend, images[chosen_idx].data, *images[chosen_idx].size, -1, -1);
}
