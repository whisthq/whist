#ifndef WHIST_SDL_UTILS_H
#define WHIST_SDL_UTILS_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file sdl_utils.h
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

#include <whist/core/whist.h>
#include "sdlscreeninfo.h"
#include "video.h"
#include "frontend/frontend.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Check whether we are going to initialize the protocol window
 * without the taskbar.
 *
 * @returns                        A copy of the skip_taskbar static variable, which is set by a
 * command line option.
 */
bool get_skip_taskbar(void);

/**
 * @brief                          Creates the SDL window,
 *                                 with the currently running thread at the owner
 *
 * @param output_width             The width of the SDL window to create, in
 *                                 pixels
 * @param output_height            The height of the SDL window to create, in
 *                                 pixels
 *
 * @param name                     The title of the window
 *
 * @param icon_filename            The filename of the window icon, pointing to a 64x64 png,
 *                                 or NULL to use the default icon.
 *
 * @returns                        NULL if fails to create SDL window, else it
 *                                 returns the SDL window variable
 */
SDL_Window* init_sdl(int output_width, int output_height, char* name, char* icon_filename,
                     WhistFrontend** frontend);

/**
 * @brief                          Destroys an SDL window and associated
 *                                 parameters
 *
 * @param window                   The SDL window to destroy
 */
void destroy_sdl(SDL_Window* window);

/**
 * @brief                          When the window gets resized, call this function
 *                                 to update the internal rendering dimensions.
 *                                 This function will also sync the server to those dimensions.
 */
void sdl_renderer_resize_window(int width, int height);

/**
 * @brief                          Updates the framebuffer to a loading screen image.
 *                                 Remember to call sdl_render_framebuffer later.
 *
 * @param idx                      The index of the animation to render.
 *                                 This should normally be strictly increasing or reset to 0.
 */
void sdl_update_framebuffer_loading_screen(int idx);

// The pixel format required for the data/linesize passed into sdl_update_framebuffer
#define SDL_FRAMEBUFFER_PIXEL_FORMAT AV_PIX_FMT_NV12

/**
 * @brief                          Update the renderer's framebuffer,
 *                                 using the provided texture.
 *
 * @param data                     The data pointers to the image
 *                                 The image must be of the format SDL_FRAMEBUFFER_PIXEL_FORMAT
 * @param linesize                 The linesize data for the image
 * @param width                    The width of the image
 * @param height                   The height of the image
 *
 * @note                           The Uint8*'s pointed to by data[] MUST be kept alive,
 *                                 until after BOTH `sdl_render_framebuffer` is called,
 *                                 AND `sdl_render_pending` subsequently returns false.
 */
void sdl_update_framebuffer(Uint8* data[4], int linesize[4], int width, int height);

/**
 * @brief                          This will mark the framebuffer as ready-to-render,
 *                                 and `update_pending_sdl_tasks` will eventually render it.
 *
 * @note                           Will make `sdl_render_pending` return true, up until
 *                                 `update_pending_sdl_tasks` is called on the main thread.
 */
void sdl_render_framebuffer(void);

/**
 * @brief                          Returns whether or not
 *                                 a framebuffer is pending to render
 *
 * @returns                        True if `sdl_render_framebuffer` has been called,
 *                                 but `update_pending_sdl_tasks` has not been called.
 */
bool sdl_render_pending(void);

/**
 * @brief                          Update the cursor
 *
 * @param cursor                   The WhistCursorInfo to use for the new cursor
 *
 * @note                           This function is virtually instantaneous and
 *                                 is not thread-safe
 */
void sdl_update_cursor(WhistCursorInfo* cursor);

/**
 * @brief                          Update the color of the window's titlebar
 *
 * @param color                    The color to set the titlebar too
 *
 * @note                           This function is virtually instantaneous
 */
void sdl_render_window_titlebar_color(WhistRGBColor color);

/**
 * @brief                          Update the title of the window
 *
 * @param window_title             The string to set the window's title to
 *
 * @note                           This function is virtually instantaneous
 */
void sdl_set_window_title(const char* window_title);

/**
 * @brief                          Update the window's fullscreen state
 *
 * @param is_fullscreen            If True, the window will become fullscreen
 *                                 If False, the window will return to windowed mode
 *
 * @note                           This function is virtually instantaneous
 */
void sdl_set_fullscreen(bool is_fullscreen);

/**
 * @brief                          Returns whether or not the window is currently visible
 *
 * @returns                        False if the window is occluded or minimized,
 *                                 True otherwise
 *
 * @note                           This function is virtually instantaneous
 */
bool sdl_is_window_visible(void);

/**
 * @brief                          The above functions may be expensive, and thus may
 *                                 have to be queued up. This function executes all of the
 *                                 currently internally queued actions.
 *
 * @note                           This function must be called by the
 *                                 same thread that originally called init_sdl.
 *                                 This will also render out any pending framebuffer,
 *                                 thereby setting `sdl_render_pending` to false.
 */
void sdl_update_pending_tasks(void);

/**
 * @brief                          Copies private variable values to the variables pointed by the
 * non-NULL pointers passed as parameters. Used for testing.
 *
 * @param pending_resize_message_ptr   If non-NULL, the function will save the value of
 * pending_resize_message in the variable pointed to by this pointer.
 *
 * @param native_window_color_is_null_ptr   If non-NULL, the function will save 'true' in the
 * variable pointed to by this pointer if native_window_color is NULL, 'false' otherwise.
 *
 * @param native_window_color_ptr   If non-NULL, the function will save the value of
 * native_window_color in the variable pointed to by this pointer.
 *
 * @param native_window_color_update_ptr   If non-NULL, the function will save the value of
 * native_window_color_update in the variable pointed to by this pointer.
 *
 * @param window_title_ptr   If non-NULL, the function will save the value of window_title in the
 * variable pointed to by this pointer.
 *
 * @param should_update_window_title_ptr   If non-NULL, the function will save the value of
 * should_update_window_title in the variable pointed to by this pointer.
 *
 * @param fullscreen_trigger_ptr   If non-NULL, the function will save the value of
 * fullscreen_trigger in the variable pointed to by this pointer.
 *
 * @param fullscreen_value_ptr   If non-NULL, the function will save the value of fullscreen_value
 * in the variable pointed to by this pointer.
 *
 */
void sdl_utils_check_private_vars(bool* pending_resize_message_ptr,
                                  bool* native_window_color_is_null_ptr,
                                  WhistRGBColor* native_window_color_ptr,
                                  bool* native_window_color_update_ptr, char* window_title_ptr,
                                  bool* should_update_window_title_ptr,
                                  bool* fullscreen_trigger_ptr, bool* fullscreen_value_ptr);

/**
 * @brief                          Called when a drag event has been detected. Uses the
 *                                 sdl global mouse and window state to initiate dragging
 *                                 indication or emulation.
 *
 */
void sdl_handle_drag_event(void);

/**
 * @brief                          Cancel or terminate drag indications/emulations
 *
 * @note                           This should be called when mouse exits the window or on mouseup
 */
void sdl_end_drag_event(void);

#endif  // WHIST_SDL_UTILS_H
