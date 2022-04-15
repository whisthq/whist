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

#ifdef _WIN32
#define COBJMACROS
#include <libavutil/hwcontext_d3d11va.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#endif

#include "sdl_utils.h"
#include <whist/utils/png.h>
#include <whist/utils/lodepng.h>
#include "frontend/frontend.h"
#include "whist/utils/command_line.h"
#include <whist/logging/log_statistic.h>

#include <whist/utils/color.h>
#include "native_window_utils.h"
#include "client_utils.h"

extern volatile int output_width;
extern volatile int output_height;
extern volatile bool insufficient_bandwidth;
volatile SDL_Window* window;
static bool skip_taskbar;

// on macOS, we must initialize the renderer in `init_sdl()` instead of video.c
static SDL_Renderer* sdl_renderer = NULL;
static WhistMutex renderer_mutex;

// pending render Update
static volatile bool pending_render = false;

static volatile bool prev_insufficient_bandwidth = false;

// Renderer D3D11 device.
#ifdef _WIN32
static ID3D11Device* renderer_d3d11_device;
#endif

// Video device and format compatible with the renderer.
static AVBufferRef* video_hardware_device = NULL;
static enum AVPixelFormat video_hardware_format = AV_PIX_FMT_NONE;

// Video texture update
static SDL_Texture* video_texture = NULL;
static SDL_PixelFormatEnum video_texture_format = SDL_PIXELFORMAT_UNKNOWN;
static AVFrame* video_frame = NULL;
static bool pending_video = false;
static AVFrame* pending_video_frame = NULL;

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

// File Drag Icon Update
static volatile bool pending_file_drag_update = false;
static int file_drag_update_x = 0;
static int file_drag_update_y = 0;

// Overlay removals
static volatile bool pending_overlay_removal = false;

/*
============================
Command-line options
============================
*/

COMMAND_LINE_BOOL_OPTION(skip_taskbar, 's', "skip-taskbar",
                         "Launch the protocol without displaying an icon in the taskbar.")

/*
============================
Private Function Declarations
============================
*/

/**
 * @brief                          Creates an SDL_Surface from a png file
 *
 * @param filename                 The filename of the png file that will be used
 *
 * @returns                        The SDL_Surface that got created from the png file
 *
 * @note                           `sdl_free_png_file_rgb_surface` must eventually be called!
 */
static SDL_Surface* sdl_surface_from_png_file(char* filename);

/**
 * @brief                          Destroys an SDL_Surface created by sdl_surface_from_png_file
 *
 * @param surface                  The SDL_Surface to free
 */
static void sdl_free_png_file_rgb_surface(SDL_Surface* surface);

/**
 * @brief                          Updates the framebuffer from any pending framebuffer call,
 *                                 Then RenderPresent if sdl_render_framebuffer is pending.
 */
static void sdl_present_pending_framebuffer(void);

/**
 * @brief                          Renders out a solid color to the framebuffer
 *
 * @param color                    The color to render
 *
 * @note                           Must be called on the main thread
 */
static void sdl_render_solid_color(WhistRGBColor color);

/**
 * @brief                          Renders out the insufficient bandwidth error message
 *
 * @note                           Must be called on the main thread
 */
static void sdl_render_insufficient_bandwidth(void);

/**
 * @brief                          Update the video texture with pending video data.
 *
 * @note                           Must be called on the main thread
 */
static void sdl_update_video(void);

/**
 * @brief                          Render the video texture to the output.
 *
 * @note                           Must be called on the main thread
 */
static void sdl_render_video(void);

/**
 * @brief                          Render a file drag indication icon
 *
 * @param x                        x position of the drag relative to the sdl window
 *
 * @param y                        y position of the drag relative to the sdl window
 *
 */
static void sdl_render_file_drag_icon(int x, int y);

/**
 * @brief                          A helper funcion that does the actual cursor rendering
 *
 * @param cursor                   The WhistCursorInfo to use for the new cursor
 *
 * @note                           This helper fucntion helps keep the logic of
 *                                 sdl_present_pending_cursor() clearer.
 */
static void sdl_present_pending_cursor_inner(WhistCursorInfo* cursor);

/**
 * Determine whether the renderer supports an associated video device,
 * and initialise it if it does.
 *
 * This must happen before the video thread runs; it will later retrieve
 * the result via sdl_get_video_device().
 */
static void sdl_init_video_device(SDL_Renderer* renderer, const char* driver_name);

/*
============================
Public Function Implementations
============================
*/
bool get_skip_taskbar(void) { return skip_taskbar; }

SDL_Window* init_sdl(int target_output_width, int target_output_height, char* name,
                     char* icon_filename, WhistFrontend** frontend) {
    /*
        Attaches the current thread to the specified current input client

        Arguments:
            target_output_width (int): The width of the SDL window to create, in pixels
            target_output_height (int): The height of the SDL window to create, in pixels
            name (char*): The title of the window
            icon_filename (char*): The filename of the window icon, pointing to a 64x64 png,
                or NULL for the default icon

        Return:
            sdl_window (SDL_Window*): NULL if fails to create SDL window, else the created SDL
                window
    */

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

    WhistFrontend* out_frontend = whist_frontend_create_sdl();
    if (out_frontend == NULL) {
        return NULL;
    }
    if (frontend) {
        *frontend = out_frontend;
    }

    renderer_mutex = whist_create_mutex();

    pending_cursor_info_mutex = whist_create_mutex();
    // set pending_cursor_info to NULL for safety, not necessary at the moment since init_sdl() is
    // only called once.
    pending_cursor_info = NULL;

    // Allow the screensaver to activate
    SDL_EnableScreenSaver();

    bool maximized = target_output_width == 0 && target_output_height == 0;

    // Grab the default display dimensions -- if the window starts maximized and
    // the user then unmaximizes (double click the titlebar on macOS), we will
    // set the default size of the unmaximized window to be 50% of the display's
    // width and height. Even if this isn't a multiple of 8, it's fine because
    // clicking the minimize button will trigger an SDL resize event.
    SDL_DisplayMode display_info;
    if (SDL_GetDesktopDisplayMode(0, &display_info)) {
        LOG_WARNING("SDL_GetCurrentDisplayMode failed: %s", SDL_GetError());
    }

    if (target_output_width == 0) {
        target_output_width = display_info.w;
    }

    if (target_output_height == 0) {
        target_output_height = display_info.h;
    }

    SDL_Window* sdl_window;

    if (skip_taskbar) {
        hide_native_window_taskbar();
    }

    // We start the window hidden to avoid glitchy-looking titlebar-content combinations while
    // the window is loading, and to prevent glitches caused by early user interaction.
    const uint32_t window_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL |
                                  SDL_WINDOW_RESIZABLE | (maximized ? SDL_WINDOW_MAXIMIZED : 0) |
                                  (skip_taskbar ? SDL_WINDOW_SKIP_TASKBAR : 0) | SDL_WINDOW_HIDDEN;

    // Simulate fullscreen with borderless always on top, so that it can still
    // be used with multiple monitors
    sdl_window = SDL_CreateWindow((name == NULL ? "Whist" : name), SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, target_output_width, target_output_height,
                                  window_flags);

    // temporary hook -- remove during refactor
    temp_frontend_set_window(out_frontend, sdl_window);
    if (!sdl_window) {
        LOG_ERROR("SDL: could not create window - exiting: %s", SDL_GetError());
        return NULL;
    }

    Uint32 flags = SDL_RENDERER_ACCELERATED;
#if VSYNC_ON
    flags |= SDL_RENDERER_PRESENTVSYNC;
#endif
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, flags);
    if (sdl_renderer == NULL) {
        LOG_FATAL("SDL: could not create renderer - exiting: %s", SDL_GetError());
    }
    SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

    SDL_RendererInfo info;
    SDL_GetRendererInfo(sdl_renderer, &info);
    LOG_INFO("SDL: Using renderer: %s", info.name);

    sdl_init_video_device(sdl_renderer, info.name);

    // Render a black screen before anything else,
    // To prevent being exposed to random colors
    pending_video = false;
    pending_render = true;
    sdl_present_pending_framebuffer();

    // Set icon, if an icon_filename was given
    if (icon_filename != NULL) {
        SDL_Surface* icon_surface = sdl_surface_from_png_file(icon_filename);
        SDL_SetWindowIcon(sdl_window, icon_surface);
        sdl_free_png_file_rgb_surface(icon_surface);
    }

    SDL_Event event;
    // Pump the event loop until the window is actually initialized (for macOS mainly).
    while (SDL_PollEvent(&event))
        ;
    // Initialize the window color to the loading background color
    set_native_window_color(sdl_window, background_color);

    // Only after the window finishes loading is it safe to initialize native options.
    init_native_window_options(sdl_window);

    // Only after the window finishes loading may we set the window minimum size
    SDL_SetWindowMinimumSize(sdl_window, MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);

    // After creating the window, we will grab DPI-adjusted dimensions in real
    // pixels
    int w, h;
    whist_frontend_get_window_pixel_size(out_frontend, &w, &h);
    output_width = w;
    output_height = h;

    // Pump the event loop until the window fully finishes loading (prevents bugs where the window
    // is permanently unfocused).
    while (SDL_PollEvent(&event))
        ;

    return sdl_window;
}

void destroy_sdl(SDL_Window* window_param, WhistFrontend* frontend) {
    /*
        Destroy the SDL resources

        Arguments:
            window_param (SDL_Window*): SDL window to be destroyed
    */

    // Destroy the video texture
    if (video_texture) {
        SDL_DestroyTexture(video_texture);
        video_texture = NULL;
    }

    // Destroy any video frames we have references to.
    av_frame_free(&video_frame);
    av_frame_free(&pending_video_frame);

    // Destroy video device.
    av_buffer_unref(&video_hardware_device);

    // Destroy the renderer
    if (sdl_renderer) {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = NULL;
    }

#ifdef _WIN32
    // Release renderer device, if it exists.
    if (renderer_d3d11_device) {
        ID3D11Device_Release(renderer_d3d11_device);

#ifndef NDEBUG
        // In debug mode, enumerate all live D3D11 objects since we
        // should have destroyed them all by now.
        IDXGIDebug* dxgi_debug;
        HRESULT hr = DXGIGetDebugInterface1(0, &IID_IDXGIDebug, &dxgi_debug);
        if (FAILED(hr)) {
            // Ignore - probably missing some debug feature (e.g. being
            // run on a device without the SDK installed).
        } else {
            IDXGIDebug_ReportLiveObjects(dxgi_debug, DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            IDXGIDebug_Release(dxgi_debug);
        }
#endif
    }
#endif  // _WIN32

    if (native_window_color) {
        free((WhistRGBColor*)native_window_color);
        native_window_color = NULL;
    }

    LOG_INFO("Destroying SDL");

    if (window_param) {
        window_param = NULL;
    }

    whist_destroy_mutex(pending_cursor_info_mutex);
    whist_destroy_mutex(renderer_mutex);

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
            SDL_SetWindowSize((SDL_Window*)window, desired_width * 96 / dpi,
                              desired_height * 96 / dpi);
            LOG_INFO("Forcing a resize from %dx%d to %dx%d", current_width, current_height,
                     desired_width, desired_height);
            whist_frontend_get_window_pixel_size(frontend, &current_width, &current_height);

            if (current_width != desired_width || current_height != desired_height) {
                LOG_WARNING(
                    "Unable to change window size to match desired dimensions using "
                    "SDL_SetWindowSize: "
                    "actual output=%dx%d, desired output=%dx%d",
                    current_width, current_height, desired_width, desired_height);
            }
        }
    }
#endif

    // Update output width / output height
    if (current_width != output_width || current_height != output_height) {
        output_width = current_width;
        output_height = current_height;
    }

    whist_lock_mutex(window_resize_mutex);
    pending_resize_message = true;
    whist_unlock_mutex(window_resize_mutex);

    LOG_INFO("Window resized to %dx%d (Actual %dx%d)", width, height, output_width, output_height);
}

static SDL_PixelFormatEnum sdl_get_pixel_format(enum AVPixelFormat pixfmt) {
    switch (pixfmt) {
        case AV_PIX_FMT_NV12:
        case AV_PIX_FMT_D3D11:
        case AV_PIX_FMT_VIDEOTOOLBOX:
            // Hardware formats all use NV12 internally.
            return SDL_PIXELFORMAT_NV12;
        case AV_PIX_FMT_YUV420P:
            return SDL_PIXELFORMAT_IYUV;
        default:
            return SDL_PIXELFORMAT_UNKNOWN;
    }
}

void sdl_update_framebuffer(AVFrame* frame) {
    whist_lock_mutex(renderer_mutex);

    // Check dimensions as a fail-safe
    if ((frame->width < 0 || frame->width > MAX_SCREEN_WIDTH) ||
        (frame->height < 0 || frame->height > MAX_SCREEN_HEIGHT)) {
        LOG_ERROR("Invalid dimensions %dx%d for video update.", frame->width, frame->height);
    } else if (sdl_get_pixel_format(frame->format) == SDL_PIXELFORMAT_UNKNOWN) {
        LOG_ERROR("Invalid format %s for video update.", av_get_pix_fmt_name(frame->format));
    } else {
        // The previous frame should have been rendered and this
        // reference removed, but free here anyway just in case.
        av_frame_free(&pending_video_frame);

        pending_video_frame = frame;
        pending_video = true;
    }

    whist_unlock_mutex(renderer_mutex);
}

void sdl_render_framebuffer(void) {
    whist_lock_mutex(renderer_mutex);
    // Mark a render as pending
    pending_render = true;
    whist_unlock_mutex(renderer_mutex);
}

bool sdl_render_pending(void) {
    whist_lock_mutex(renderer_mutex);
    bool pending_render_val = pending_render;
    whist_unlock_mutex(renderer_mutex);
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

void sdl_present_pending_cursor(void) {
    WhistTimer statistics_timer;
    WhistCursorInfo* temp_cursor_info = NULL;

    // if there is a pending curor, take the ownership of pending_cursor_info, and assgin it to the
    // local pointer. do rendering with the local pointer after unlock, to minimize locking.
    whist_lock_mutex(pending_cursor_info_mutex);
    if (pending_cursor_info) {
        temp_cursor_info = pending_cursor_info;
        pending_cursor_info = NULL;
    }
    whist_unlock_mutex(pending_cursor_info_mutex);

    if (temp_cursor_info != NULL) {
        TIME_RUN(sdl_present_pending_cursor_inner(temp_cursor_info), VIDEO_CURSOR_UPDATE_TIME,
                 statistics_timer);
        // Cursors need not be double-rendered, so we just unset the cursor image here.
        // The duty of ree a rendered cursor is at consumer side (here), since the ownership is
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

bool sdl_is_window_visible(void) {
    return !(SDL_GetWindowFlags((SDL_Window*)window) &
             (SDL_WINDOW_OCCLUDED | SDL_WINDOW_MINIMIZED));
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
        if (fullscreen_value) {
            SDL_SetWindowFullscreen((SDL_Window*)window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
            SDL_SetWindowFullscreen((SDL_Window*)window, 0);
        }
        fullscreen_trigger = false;
    }

    // Handle any pending window titlebar color events
    if (native_window_color_update && native_window_color) {
        set_native_window_color((SDL_Window*)window, *(WhistRGBColor*)native_window_color);
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

    sdl_present_pending_cursor();

    sdl_present_pending_framebuffer();
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

void sdl_handle_drag_event(WhistFrontend* frontend) {
    /*
      Initiates the rendering of the drag icon by checking if the drag is occuring within
      the sdl window and then setting the correct state variables for pending_file_drag_update
      This will spam logs pretty badly when dragging is active - so avoiding that here.

      exception of native_window_color_is_null_ptr, which has a slightly different purpose) with the
      values held by the corresponding sdl_utils.c globals. If native_window_color_is_null_ptr is
      not NULL, we set the value pointed to by it with a boolean indicating whether the
      native_window_color global pointer is NULL.
     */

    int x_window, y_window, w_window, h_window, x_mouse_global, y_mouse_global;
    whist_frontend_get_window_position(frontend, &x_window, &y_window);
    whist_frontend_get_window_virtual_size(frontend, &w_window, &h_window);
    // Mouse is not active within window - so we must use the global mouse and manually transform
    whist_frontend_get_global_mouse_position(frontend, &x_mouse_global, &y_mouse_global);

    if (x_window < x_mouse_global && x_mouse_global < x_window + w_window &&
        y_window < y_mouse_global && y_mouse_global < y_window + h_window) {
        // Scale relative global mouse offset to output window
        file_drag_update_x = output_width * (x_mouse_global - x_window) / w_window;
        file_drag_update_y = output_height * (y_mouse_global - y_window) / h_window;
        pending_file_drag_update = true;
    } else {
        // Stop the rendering of the file drag icon if event has left the window
        sdl_end_drag_event();
    }
}

void sdl_end_drag_event() {
    /*
        Setting pending_file_drag_update false will prevent
        the file icon from being displayed in the next sdl step
    */

    pending_file_drag_update = false;
    // Render the next frame to remove the file drag icon
    pending_overlay_removal = true;
}

WhistStatus sdl_get_video_device(AVBufferRef** device, enum AVPixelFormat* format) {
    // These are set when SDL is initialised before anything
    // video-related has started, so not further synchronisation is
    // required here.
    *device = video_hardware_device;
    *format = video_hardware_format;
    return WHIST_SUCCESS;
}

/*
============================
Private Function Implementations
============================
*/

static void sdl_present_pending_framebuffer(void) {
    // Render out the current framebuffer, if there's a pending render
    whist_lock_mutex(renderer_mutex);

    // Render the error message immediately during state transition to insufficient bandwidth
    if (prev_insufficient_bandwidth == false && insufficient_bandwidth == true) {
        pending_render = true;
    }

    // If there's no pending render or overlay visualizations, just do nothing,
    // Don't consume and discard any pending nv12 or loading screen.
    if (!pending_render && !pending_file_drag_update && !pending_overlay_removal) {
        whist_unlock_mutex(renderer_mutex);
        return;
    }

    // Wipes the renderer to background color before we present
    sdl_render_solid_color(background_color);

    WhistTimer statistics_timer;
    start_timer(&statistics_timer);

    // Update the video texture if new video is pending.
    if (pending_video) {
        sdl_update_video();
    }

    // Render video if possible.
    sdl_render_video();

    if (insufficient_bandwidth) {
        sdl_render_insufficient_bandwidth();
    }
    prev_insufficient_bandwidth = insufficient_bandwidth;

    // Render the drag icon if a drag update has been signaled
    if (pending_file_drag_update) {
        sdl_render_file_drag_icon(file_drag_update_x, file_drag_update_y);
    }

    log_double_statistic(VIDEO_RENDER_TIME, get_timer(&statistics_timer) * MS_IN_SECOND);
    whist_unlock_mutex(renderer_mutex);

    // RenderPresent outside of the mutex, since RenderCopy made a copy anyway
    // and this will take ~8ms if VSYNC is on.
    // (If this causes a bug, feel free to pull back to inside of the mutex)
    TIME_RUN(SDL_RenderPresent(sdl_renderer), VIDEO_RENDER_TIME, statistics_timer);

    whist_lock_mutex(renderer_mutex);
    pending_render = false;
    pending_overlay_removal = false;
    whist_unlock_mutex(renderer_mutex);
}

static void sdl_render_solid_color(WhistRGBColor color) {
    SDL_SetRenderDrawColor(sdl_renderer, color.red, color.green, color.blue, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);
}

static void sdl_render_insufficient_bandwidth(void) {
    const char* filenames[] = {"error_message_500x50.png", "error_message_750x75.png",
                               "error_message_1000x100.png", "error_message_1500x150.png"};
    // Width in pixels for the above files. Maintain the below list in ascending order.
    const int file_widths[] = {500, 750, 1000, 1500};
#define TARGET_WIDTH_IN_INCHES 5.0
    int chosen_idx = 0;
    int dpi = get_native_window_dpi((SDL_Window*)window);
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

    SDL_Surface* error_message = sdl_surface_from_png_file(frame_filename);
    if (error_message == NULL) {
        LOG_ERROR("Insufficient Bandwidth image failed to load");
    } else {
        SDL_Texture* error_message_texture =
            SDL_CreateTextureFromSurface(sdl_renderer, error_message);

        // The surface can now be freed
        sdl_free_png_file_rgb_surface(error_message);

        // Position the rectangle such that the texture will be centered
        int w, h;
        SDL_QueryTexture(error_message_texture, NULL, NULL, &w, &h);
        SDL_Rect centered_rect = {
            .x = output_width / 2 - w / 2,
            .y = output_height - h,
            .w = w,
            .h = h,
        };
        // Now, we write the texture out to the renderer
        SDL_RenderCopy(sdl_renderer, error_message_texture, NULL, &centered_rect);

        SDL_DestroyTexture(error_message_texture);
    }
}

static void sdl_update_video(void) {
    int res;

    FATAL_ASSERT(pending_video == true);

    AVFrame* frame = pending_video_frame;
    FATAL_ASSERT(frame);

    SDL_PixelFormatEnum format = sdl_get_pixel_format(frame->format);
    FATAL_ASSERT(format != SDL_PIXELFORMAT_UNKNOWN);

    // Formats we support for texture import to SDL.
    bool import_texture =
        (frame->format == AV_PIX_FMT_VIDEOTOOLBOX || frame->format == AV_PIX_FMT_D3D11);

    if (import_texture || format != video_texture_format) {
        // When importing we will make a new SDL texture referring to
        // the imported one, so the old texture should be destroyed.
        // When uploading this is only needed if the format changes.
        if (video_texture) {
            SDL_DestroyTexture(video_texture);
            video_texture = NULL;
        }
    }
    // The old video data is no longer needed.
    av_frame_free(&video_frame);

    if (import_texture) {
        SDL_TextureHandleD3D11 d3d11_handle = {0};
        void* handle;

        if (frame->format == AV_PIX_FMT_VIDEOTOOLBOX) {
            // This is a pointer to the CVPixelBuffer containing the
            // frame data.
            handle = frame->data[3];
        } else if (frame->format == AV_PIX_FMT_D3D11) {
#ifdef _WIN32
            // This is a pointer to a texture, but it exists on the
            // video decode device rather than the render device.  Make
            // a new reference on the render device to give to SDL.
            ID3D11Texture2D* texture = (ID3D11Texture2D*)frame->data[0];
            int index = (intptr_t)frame->data[1];

            HRESULT hr;
            IDXGIResource1* resource;
            hr = ID3D11Texture2D_QueryInterface(texture, &IID_IDXGIResource1, &resource);
            if (FAILED(hr)) {
                LOG_ERROR("Failed to get DXGI resource for video texture: %#x.", hr);
                goto fail;
            }

            HANDLE shared_handle;
            hr = IDXGIResource1_CreateSharedHandle(resource, NULL, DXGI_SHARED_RESOURCE_READ, NULL,
                                                   &shared_handle);
            IDXGIResource1_Release(resource);
            if (FAILED(hr)) {
                LOG_ERROR("Failed to create shared handle for video texture: %#x.", hr);
                goto fail;
            }

            ID3D11Device1* device1;
            hr = ID3D11Device_QueryInterface(renderer_d3d11_device, &IID_ID3D11Device1, &device1);
            if (FAILED(hr)) {
                LOG_ERROR("Failed to get D3D11 device1 for device: %#x.", hr);
                CloseHandle(shared_handle);
                goto fail;
            }

            ID3D11Texture2D* render_texture;
            hr = ID3D11Device1_OpenSharedResource1(device1, shared_handle, &IID_ID3D11Texture2D,
                                                   &render_texture);
            ID3D11Device1_Release(device1);
            CloseHandle(shared_handle);
            if (FAILED(hr)) {
                LOG_ERROR("Failed to open shared handle for renderer texture: %#x.", hr);
                goto fail;
            }

            d3d11_handle.texture = render_texture;
            d3d11_handle.index = index;
            handle = &d3d11_handle;
#else
            LOG_FATAL("D3D11 support requires Windows.");
#endif
        } else {
            LOG_FATAL("Invalid format %s for texture import.", av_get_pix_fmt_name(frame->format));
        }

        // The texture dimensions match the coded size of the video, not
        // the cropped size.
        int texture_width = FFALIGN(frame->width, 16);
        int texture_height = FFALIGN(frame->height, 16);

        video_texture = SDL_CreateTextureFromHandle(sdl_renderer, format, SDL_TEXTUREACCESS_STATIC,
                                                    texture_width, texture_height, handle);
        if (video_texture == NULL) {
            LOG_ERROR("Failed to import video texture: %s.", SDL_GetError());
            goto fail;
        }

#ifdef _WIN32
        if (frame->format == AV_PIX_FMT_D3D11) {
            // Release the render texture - the SDL texture now contains
            // a reference to it.
            ID3D11Texture2D_Release((ID3D11Texture2D*)d3d11_handle.texture);
        }
#endif
    } else {
        if (video_texture == NULL) {
            // Create new video texture.
            video_texture = SDL_CreateTexture(sdl_renderer, format, SDL_TEXTUREACCESS_STREAMING,
                                              MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);
            if (video_texture == NULL) {
                LOG_ERROR("Failed to create %s video texture: %s.",
                          av_get_pix_fmt_name(frame->format), SDL_GetError());
                goto fail;
            }

            LOG_INFO("Using %s video texture.", av_get_pix_fmt_name(frame->format));
            video_texture_format = format;
        }

        // The texture object we allocate is larger than the frame,
        // so we only copy the valid section of the frame into the texture.
        SDL_Rect texture_rect = {
            .x = 0,
            .y = 0,
            .w = frame->width,
            .h = frame->height,
        };
        if (frame->format == AV_PIX_FMT_NV12) {
            res = SDL_UpdateNVTexture(video_texture, &texture_rect, frame->data[0],
                                      frame->linesize[0], frame->data[1], frame->linesize[1]);
        } else if (frame->format == AV_PIX_FMT_YUV420P) {
            res = SDL_UpdateYUVTexture(video_texture, &texture_rect, frame->data[0],
                                       frame->linesize[0], frame->data[1], frame->linesize[1],
                                       frame->data[2], frame->linesize[2]);
        } else {
            LOG_FATAL("Invalid format %s for texture update.", av_get_pix_fmt_name(frame->format));
        }

        if (res < 0) {
            LOG_ERROR("Failed to update texture from %s frame: %s.",
                      av_get_pix_fmt_name(frame->format), SDL_GetError());
            goto fail;
        }
    }

    // Move reference: the lifetime of this frame wants to matcn the
    // texture associated with it.
    video_frame = pending_video_frame;
    pending_video_frame = NULL;

fail:
    pending_video = false;
}

static void sdl_render_video(void) {
    // Take the subsection of texture that should be rendered to screen,
    // And draw it on the renderer
    if (video_texture == NULL) {
        // We must be attempting to render before any frame has been
        // decoded.  Just do nothing - the background colour will be
        // drawn instead.
        return;
    }
    FATAL_ASSERT(video_frame);

// On macOS & Windows, SDL outputs the texture with the last pixel on the bottom and
// right sides without data, rendering it green (NV12 color format). We're not sure
// why that is the case, but in the meantime, clipping that pixel makes the visual
// look seamless.
#if defined(__APPLE__) || defined(_WIN32)
#define CLIPPED_PIXELS 1
#else
#define CLIPPED_PIXELS 0
#endif

    SDL_Rect output_rect = {
        .x = 0,
        .y = 0,
        .w = min(output_width, video_frame->width) - CLIPPED_PIXELS,
        .h = min(output_height, video_frame->height) - CLIPPED_PIXELS,
    };
    int res = SDL_RenderCopy(sdl_renderer, video_texture, &output_rect, NULL);
    if (res < 0) {
        LOG_ERROR("Failed to render video texture: %s.", SDL_GetError());
    }
}

static SDL_Surface* sdl_surface_from_png_file(char* filename) {
    unsigned int w, h, error;
    unsigned char* image;

    // decode to 32-bit RGBA
    // TODO: We need to free image after destroying the SDL_Surface!
    // Perhaps we should simply return a PNG buffer and have the caller
    // manage memory allocation.
    error = lodepng_decode32_file(&image, &w, &h, filename);
    if (error) {
        LOG_ERROR("decoder error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

// masks for little-endian RGBA
#define RGBA_MASK_A 0xff000000
#define RGBA_MASK_B 0x00ff0000
#define RGBA_MASK_G 0x0000ff00
#define RGBA_MASK_R 0x000000ff

    // buffer pointer, width, height, bits per pixel, bytes per row, R/G/B/A masks
    SDL_Surface* surface =
        SDL_CreateRGBSurfaceFrom(image, w, h, sizeof(uint32_t) * 8, sizeof(uint32_t) * w,
                                 RGBA_MASK_R, RGBA_MASK_G, RGBA_MASK_B, RGBA_MASK_A);

    if (surface == NULL) {
        LOG_ERROR("Failed to load SDL surface from file '%s': %s", filename, SDL_GetError());
        free(image);
        return NULL;
    }

    return surface;
}

static void sdl_free_png_file_rgb_surface(SDL_Surface* surface) {
    // Specified to call `SDL_FreeSurface` on the surface, and `free` on the pixels array
    char* pixels = surface->pixels;
    SDL_FreeSurface(surface);
    free(pixels);
}

// 20th of the window - usually close to file icon sizes + doesn't become disruptive if window is
// small
#define DRAG_ICON_SCALE 0.05
static void sdl_render_file_drag_icon(int x, int y) {
    SDL_Surface* drop_icon_surface = sdl_surface_from_png_file("images/file_drag_icon.png");
    SDL_Texture* drop_icon_texture = SDL_CreateTextureFromSurface(sdl_renderer, drop_icon_surface);
    sdl_free_png_file_rgb_surface(drop_icon_surface);
    // Shift icon in direction away from closest out of bounds
    int horizontal_direction = (x < output_width / 2) ? 1 : -2;
    int vertical_direction = -1;
    double icon_size = min(output_width, output_height) * DRAG_ICON_SCALE;
    SDL_Rect drop_icon_rect = {.x = x + horizontal_direction * icon_size,
                               .y = y + vertical_direction * icon_size / 2,
                               .w = icon_size,
                               .h = icon_size};

    SDL_RenderCopy(sdl_renderer, drop_icon_texture, NULL, &drop_icon_rect);
    SDL_DestroyTexture(drop_icon_texture);
}

static void sdl_present_pending_cursor_inner(WhistCursorInfo* cursor) {
    /*
      Update the cursor image on the screen. If the cursor hasn't changed since the last frame we
      received, we don't do anything. Otherwise, we either use the provided bitmap or update the
      cursor ID to tell SDL which cursor to render.
     */
    // Set cursor to frame's desired cursor type
    // Only update the cursor, if a cursor image is even embedded in the frame at all.

    // The PNG decodes to RGBA
#define CURSORIMAGE_R 0xff000000
#define CURSORIMAGE_G 0x00ff0000
#define CURSORIMAGE_B 0x0000ff00
#define CURSORIMAGE_A 0x000000ff

    static WhistCursorState last_cursor_state = CURSOR_STATE_VISIBLE;
    static struct {
        int x;
        int y;
    } last_visible_cursor_position = {0, 0};

    static uint32_t last_cursor_hash = 0;
    static SDL_Cursor* cleanup_cursor = NULL;

    if (cursor == NULL) {
        return;
    }

    if (cursor->hash != last_cursor_hash) {
        SDL_Cursor* sdl_cursor = NULL;
        // Render new cursor
        if (cursor->using_png) {
            uint8_t* rgba = whist_cursor_info_to_rgba(cursor);
            // use png data to set cursor
            SDL_Surface* cursor_surface = SDL_CreateRGBSurfaceFrom(
                rgba, cursor->png_width, cursor->png_height, sizeof(uint32_t) * 8,
                sizeof(uint32_t) * cursor->png_width, CURSORIMAGE_R, CURSORIMAGE_G, CURSORIMAGE_B,
                CURSORIMAGE_A);

            // Use BLENDMODE_NONE to allow for proper cursor blit-resize
            SDL_SetSurfaceBlendMode(cursor_surface, SDL_BLENDMODE_NONE);

#ifdef _WIN32
            // on Windows, the cursor DPI is unchanged
            const int cursor_dpi = 96;
#else
            // on other platforms, use the window DPI
            const int cursor_dpi = get_native_window_dpi((SDL_Window*)window);
#endif  // _WIN32

            // Create the scaled cursor surface which takes DPI into account
            SDL_Surface* scaled_cursor_surface = SDL_CreateRGBSurface(
                0, cursor->png_width * 96 / cursor_dpi, cursor->png_height * 96 / cursor_dpi,
                cursor_surface->format->BitsPerPixel, cursor_surface->format->Rmask,
                cursor_surface->format->Gmask, cursor_surface->format->Bmask,
                cursor_surface->format->Amask);

            // Copy the original cursor into the scaled surface
            SDL_BlitScaled(cursor_surface, NULL, scaled_cursor_surface, NULL);

            // Release the original cursor surface
            SDL_FreeSurface(cursor_surface);
            free(rgba);

            // Potentially SDL_SetSurfaceBlendMode here since X11 cursor BMPs are
            // pre-alpha multplied. Remember to adjust hot_x/y by the DPI scaling.
            sdl_cursor =
                SDL_CreateColorCursor(scaled_cursor_surface, cursor->png_hot_x * 96 / cursor_dpi,
                                      cursor->png_hot_y * 96 / cursor_dpi);
            SDL_FreeSurface(scaled_cursor_surface);
        } else {
            // use cursor id to set cursor
            sdl_cursor = SDL_CreateSystemCursor((SDL_SystemCursor)cursor->cursor_id);
        }
        SDL_SetCursor(sdl_cursor);

        // SDL_SetCursor only works as long as the currently set cursor still exists.
        // Hence, we maintain a reference to the currently set cursor and only clean it up
        // after a new cursor is created.
        if (cleanup_cursor) {
            SDL_FreeCursor(cleanup_cursor);
        }
        cleanup_cursor = sdl_cursor;

        last_cursor_hash = cursor->hash;
    }

    if (cursor->cursor_state != last_cursor_state) {
        if (cursor->cursor_state == CURSOR_STATE_HIDDEN) {
            // Capture the coordinates of the cursor so we can restore it when it becomes visible
            // again
            SDL_GetGlobalMouseState(&last_visible_cursor_position.x,
                                    &last_visible_cursor_position.y);
            SDL_SetRelativeMouseMode(true);
            LOG_INFO("CURSOR GOING HIDDEN AT %d %d", last_visible_cursor_position.x,
                     last_visible_cursor_position.y);
        } else {
            // The cursor became visible, so move it to where it was captured.
            SDL_SetRelativeMouseMode(false);
            SDL_WarpMouseGlobal(last_visible_cursor_position.x, last_visible_cursor_position.y);
            LOG_INFO("CURSOR GOING VISIBLE AT %d %d", last_visible_cursor_position.x,
                     last_visible_cursor_position.y);
        }
        last_cursor_state = cursor->cursor_state;
    }
}

static void sdl_init_video_device(SDL_Renderer* renderer, const char* driver_name) {
    int err;

#ifdef _WIN32
    HRESULT hr;
    if (!strcmp(driver_name, "direct3d11")) {
        ID3D11Device* d3d11_device = SDL_RenderGetD3D11Device(renderer);
        if (d3d11_device == NULL) {
            LOG_WARNING("Failed to fetch D3D11 device: %s.", SDL_GetError());
            return;
        }

        LOG_INFO("Using D3D11 device from SDL renderer.");
        renderer_d3d11_device = d3d11_device;

        // Make an independent reference to the device so we have a
        // separate immediate context to use on the video thread.

        IDXGIDevice* dxgi_device;
        hr = ID3D11Device_QueryInterface(d3d11_device, &IID_IDXGIDevice, &dxgi_device);
        if (FAILED(hr)) {
            LOG_WARNING("Failed to get DXGI device from D3D11 device: %#x.", hr);
            return;
        }
        IDXGIAdapter* dxgi_adapter;
        hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);
        if (FAILED(hr)) {
            LOG_WARNING("Failed to get DXGI adapter from DXGI device: %#x.", hr);
            return;
        }

        AVBufferRef* dev_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
        FATAL_ASSERT(dev_ref);
        AVHWDeviceContext* dev = (AVHWDeviceContext*)dev_ref->data;
        AVD3D11VADeviceContext* hwctx = dev->hwctx;

        UINT device_flags = 0;
#ifndef NDEBUG
        device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        hr = D3D11CreateDevice(dxgi_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, device_flags, NULL, 0,
                               D3D11_SDK_VERSION, &hwctx->device, NULL, &hwctx->device_context);
        IDXGIAdapter_Release(dxgi_adapter);
        if (FAILED(hr)) {
            LOG_WARNING("Failed to create D3D11 device for video: %#x.", hr);
            return;
        }

        err = av_hwdevice_ctx_init(dev_ref);
        if (err < 0) {
            LOG_WARNING("Failed to init D3D11 hardware device for video: %s.", av_err2str(err));
            av_buffer_unref(&dev_ref);
            return;
        }

        video_hardware_device = dev_ref;
        video_hardware_format = AV_PIX_FMT_D3D11;
    }
#endif

#ifdef __APPLE__
    if (!strcmp(driver_name, "metal")) {
        // No device required; output can use Core Video pixel buffers
        // from VideoToolbox.
        video_hardware_format = AV_PIX_FMT_VIDEOTOOLBOX;
    }
#endif

    // More:
    // * OpenGL on Linux can work with VAAPI via DRM.
    // * D3D9 can work with DXVA2.
    // * Nvidia devices might work with NVDEC somehow?
}
