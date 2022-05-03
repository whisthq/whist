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
#include <whist/utils/png.h>
#include <whist/utils/lodepng.h>
#include "frontend/frontend.h"
#include <whist/logging/log_statistic.h>
#include <whist/utils/command_line.h>

#include <whist/utils/color.h>
#include "client_utils.h"

extern volatile int output_x;
extern volatile int output_y;
extern volatile int output_width;
extern volatile int output_height;
extern volatile int display_width;
extern volatile int display_height;
extern volatile bool insufficient_bandwidth;

static WhistMutex frontend_render_mutex;

// pending render Update
static volatile bool pending_render = false;

static volatile bool prev_insufficient_bandwidth = false;

// Video frame update.
static AVFrame* pending_video_frame;

// for cursor update. The value is writen by the video render thread, and taken away by the main
// thread.
static WhistCursorInfo* pending_cursor_info = NULL;
// the mutex to protect pending_cursor_info
static WhistMutex pending_cursor_info_mutex;

// The background color for the loading screen
static const WhistRGBColor background_color = {17, 24, 39};  // #111827 (thanks copilot)

// Window Color Update
static volatile WhistRGBColor* native_window_color = NULL;
static volatile bool native_window_color_update = false;

// Window Title Update
static volatile char* window_title = NULL;
static volatile bool should_update_window_title = false;

// Full Screen Update
static volatile bool fullscreen_trigger = false;
static volatile bool fullscreen_value = false;

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

WhistFrontend* init_sdl(int target_output_width, int target_output_height, const char* title) {
    WhistFrontend* frontend = whist_frontend_create(frontend_type ? frontend_type : "sdl");
    if (frontend == NULL) {
        LOG_ERROR("Failed to create frontend");
        return NULL;
    }

    if (whist_frontend_init(frontend, target_output_width, target_output_height, title,
                            &background_color) != WHIST_SUCCESS) {
        whist_frontend_destroy(frontend);
        LOG_ERROR("Failed to initialize frontend");
        return NULL;
    }

    pending_cursor_info_mutex = whist_create_mutex();
    frontend_render_mutex = whist_create_mutex();
    // set pending_cursor_info to NULL for safety, not necessary at the moment since init_sdl() is
    // only called once.
    pending_cursor_info = NULL;

    pending_video_frame = NULL;
    pending_render = false;

    // After creating the window, we will grab DPI-adjusted dimensions in real
    // pixels
    int w, h;
    whist_frontend_get_window_pixel_size(frontend, &w, &h);
    display_width = w;
    display_height = h;
    return frontend;
}

void destroy_sdl(WhistFrontend* frontend) {
    if (native_window_color) {
        free((WhistRGBColor*)native_window_color);
        native_window_color = NULL;
    }

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
    whist_frontend_get_window_pixel_size(frontend, &current_width, &current_height);

    LOG_INFO("Received resize event for %dx%d, currently %dx%d", width, height, current_width,
             current_height);

#ifndef __linux__
    int dpi = whist_frontend_get_window_dpi(frontend);

    // The server will round the dimensions up in order to satisfy the YUV pixel format
    // requirements. Specifically, it will round the width up to a multiple of 8 and the height up
    // to a multiple of 2. Here, we try to force the window size to be valid values so the
    // dimensions of the client and server match. We round down rather than up to avoid extending
    // past the size of the display.
    int desired_width = current_width - (current_width % 8);
    int desired_height = current_height - (current_height % 2);
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
            whist_frontend_resize_window(frontend, desired_width * 96 / dpi,
                                         desired_height * 96 / dpi);
            LOG_INFO("Forcing a resize from %dx%d to %dx%d", current_width, current_height,
                     desired_width, desired_height);
            whist_frontend_get_window_pixel_size(frontend, &current_width, &current_height);

            if (current_width != desired_width || current_height != desired_height) {
                LOG_WARNING("Failed to force resize -- got %dx%d instead of desired %dx%d",
                            current_width, current_height, desired_width, desired_height);
            }
        }
    }
#endif

    /*
    // Update output width / output height
    if (current_width != output_width || current_height != output_height) {
        output_width = current_width;
        output_height = current_height;
    }
    */

    whist_lock_mutex(window_resize_mutex);
    pending_resize_message = true;
    whist_unlock_mutex(window_resize_mutex);

    LOG_INFO("Window resized to %dx%d (Actual %dx%d)", width, height, output_width, output_height);
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

void sdl_set_cursor_info_as_pending(WhistCursorInfo* cursor_info) {
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

void sdl_render_window_titlebar_color(WhistRGBColor color) {
    /*
      Update window titlebar color using the colors of the new frame
     */
    WhistRGBColor* current_color = (WhistRGBColor*)native_window_color;
    if (current_color != NULL) {
        if (current_color->red != color.red || current_color->green != color.green ||
            current_color->blue != color.blue) {
            // delete the old color we were using
            free(current_color);

            // make the new color and signal that we're ready to update
            WhistRGBColor* new_native_window_color = safe_malloc(sizeof(WhistRGBColor));
            *new_native_window_color = color;
            native_window_color = new_native_window_color;
            native_window_color_update = true;
        }
    } else {
        // make the new color and signal that we're ready to update
        WhistRGBColor* new_native_window_color = safe_malloc(sizeof(WhistRGBColor));
        *new_native_window_color = color;
        native_window_color = new_native_window_color;
        native_window_color_update = true;
    }
}

void sdl_set_window_title(const char* requested_window_title) {
    if (should_update_window_title) {
        LOG_WARNING(
            "Failed to update window title, as the previous window title update is still pending");
        return;
    }

    size_t len = strlen(requested_window_title) + 1;
    char* new_window_title = safe_malloc(len);
    safe_strncpy(new_window_title, requested_window_title, len);
    window_title = new_window_title;

    should_update_window_title = true;
}

void sdl_set_fullscreen(bool is_fullscreen) {
    fullscreen_trigger = true;
    fullscreen_value = is_fullscreen;
}

void sdl_update_pending_tasks(WhistFrontend* frontend) {
    // Handle any pending window title updates
    if (should_update_window_title) {
        if (window_title) {
            whist_frontend_set_title(frontend, (const char*)window_title);
            free((void*)window_title);
            window_title = NULL;
        } else {
            LOG_ERROR("Window Title should not be null!");
        }
        should_update_window_title = false;
    }

    // Handle any pending fullscreen events
    if (fullscreen_trigger) {
        whist_frontend_set_window_fullscreen(frontend, fullscreen_value);
        fullscreen_trigger = false;
    } 

    // Handle any pending window titlebar color events
    if (native_window_color_update && native_window_color) {
        whist_frontend_set_titlebar_color(frontend, (WhistRGBColor*)native_window_color);
        native_window_color_update = false;
    }

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

void sdl_utils_check_private_vars(bool* pending_resize_message_ptr,
                                  bool* native_window_color_is_null_ptr,
                                  WhistRGBColor* native_window_color_ptr,
                                  bool* native_window_color_update_ptr, char* window_title_ptr,
                                  bool* should_update_window_title_ptr,
                                  bool* fullscreen_trigger_ptr, bool* fullscreen_value_ptr) {
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

    if (native_window_color_is_null_ptr && native_window_color_ptr) {
        if (!native_window_color) {
            *native_window_color_is_null_ptr = true;
        } else {
            *native_window_color_is_null_ptr = false;
            native_window_color_ptr->red = native_window_color->red;
            native_window_color_ptr->green = native_window_color->green;
            native_window_color_ptr->blue = native_window_color->blue;
        }
    }

    if (native_window_color_update_ptr) {
        *native_window_color_update_ptr = native_window_color_update;
    }

    if (window_title_ptr) {
        size_t len = 0;
        // While loop needed for string copy because window_title is volatile
        while (window_title && window_title[len] != '\0') {
            window_title_ptr[len] = window_title[len];
            len += 1;
        }
        len += 1;
        window_title_ptr[len] = '\0';
    }

    if (should_update_window_title_ptr) {
        *should_update_window_title_ptr = should_update_window_title;
    }

    if (fullscreen_trigger_ptr) {
        *fullscreen_trigger_ptr = fullscreen_trigger;
    }
    if (fullscreen_value_ptr) {
        *fullscreen_value_ptr = fullscreen_value;
    }
}

/*
============================
Private Function Implementations
============================
*/

static void sdl_present_pending_framebuffer(WhistFrontend* frontend) {
    // Render out the current framebuffer, if there's a pending render
    whist_lock_mutex(frontend_render_mutex);

    // Render the error message immediately during state transition to insufficient bandwidth
    if (prev_insufficient_bandwidth == false && insufficient_bandwidth == true) {
        pending_render = true;
    }

    // If there's no pending render or overlay visualizations, just do nothing,
    // Don't consume and discard any pending nv12 or loading screen.
    if (!pending_render) {
        whist_unlock_mutex(frontend_render_mutex);
        return;
    }

    // Wipes the renderer to background color before we present
    whist_frontend_paint_solid(frontend, &background_color);

    WhistTimer statistics_timer;
    start_timer(&statistics_timer);

    // If there is a new video frame then update the frontend texture
    // with it.
    if (pending_video_frame) {
        whist_frontend_update_video(frontend, pending_video_frame);

        // If the frontend needs to take a reference to the frame data
        // then it has done so, so we can free this frame immediately.
        av_frame_free(&pending_video_frame);
    }

    whist_frontend_paint_video(frontend, output_x, output_y, output_width, output_height);

    if (insufficient_bandwidth) {
        render_insufficient_bandwidth(frontend);
    }
    prev_insufficient_bandwidth = insufficient_bandwidth;

    log_double_statistic(VIDEO_RENDER_TIME, get_timer(&statistics_timer) * MS_IN_SECOND);
    whist_unlock_mutex(frontend_render_mutex);

    // RenderPresent outside of the mutex, since RenderCopy made a copy anyway
    // and this will take ~8ms if VSYNC is on.
    // (If this causes a bug, feel free to pull back to inside of the mutex)
    TIME_RUN(whist_frontend_render(frontend), VIDEO_RENDER_TIME, statistics_timer);

    whist_lock_mutex(frontend_render_mutex);
    pending_render = false;
    whist_unlock_mutex(frontend_render_mutex);
}

static void render_insufficient_bandwidth(WhistFrontend* frontend) {
    const char* filenames[] = {"error_message_500x50.png", "error_message_750x75.png",
                               "error_message_1000x100.png", "error_message_1500x150.png"};
    // Width in pixels for the above files. Maintain the below list in ascending order.
    const int file_widths[] = {500, 750, 1000, 1500};
#define TARGET_WIDTH_IN_INCHES 5.0
    int chosen_idx = 0;
    int dpi = whist_frontend_get_window_dpi(frontend);
    // Choose an image closest to TARGET_WIDTH_IN_INCHES.
    // And it should be at least TARGET_WIDTH_IN_INCHES.
    for (int i = 0; i < (int)(sizeof(file_widths) / sizeof(*file_widths)); i++) {
        double width_in_inches = (double)file_widths[i] / dpi;
        if (width_in_inches > TARGET_WIDTH_IN_INCHES) {
            chosen_idx = i;
            break;
        }
    }

    char frame_filename[256];
    snprintf(frame_filename, sizeof(frame_filename), "images/%s", filenames[chosen_idx]);
    whist_frontend_paint_png(frontend, frame_filename, output_width, output_height, -1, -1);
}
