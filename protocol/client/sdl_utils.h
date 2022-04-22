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
#include "video.h"
#include "frontend/frontend.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Creates the SDL window frontend,
 *                                 with the currently running thread at the owner
 *
 * @param output_width             The width of the SDL window to create, in
 *                                 pixels
 * @param output_height            The height of the SDL window to create, in
 *                                 pixels
 *
 * @param title                    The title of the window
 *
 * @returns                        A handle for the created frontend
 *
 */
WhistFrontend* init_sdl(int output_width, int output_height, const char* title);

/**
 * @brief                          Destroys an SDL window and associated
 *                                 parameters
 *
 * @param frontend                 The frontend to be destroyed
 */
void destroy_sdl(WhistFrontend* frontend);

/**
 * @brief                          When the window gets resized, call this function
 *                                 to update the internal rendering dimensions.
 *                                 This function will also sync the server to those dimensions.
 */
void sdl_renderer_resize_window(WhistFrontend* frontend, int width, int height);

/**
 * @brief                          Updates the framebuffer to a loading screen image.
 *                                 Remember to call sdl_render_framebuffer later.
 *
 * @param idx                      The index of the animation to render.
 *                                 This should normally be strictly increasing or reset to 0.
 */
void sdl_update_framebuffer_loading_screen(int idx);

// The pixel format required for the data/linesize passed into sdl_update_framebuffer
#define WHIST_CLIENT_FRAMEBUFFER_PIXEL_FORMAT AV_PIX_FMT_NV12

/**
 * @brief                          Update the renderer's framebuffer,
 *                                 using the provided frame.
 *
 * @param frame                    Decoded frame to update the framebuffer texture with.
 *                                 The format of this frame must match
 *                                 WHIST_CLIENT_FRAMEBUFFER_PIXEL_FORMAT.
 *
 * @note                           The renderer takes ownership of the frame after this
 *                                 call.  If that is not desirable, a copy must be made
 *                                 beforehand.
 */
void sdl_update_framebuffer(AVFrame* frame);

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
 * @brief                          Set the cursor info as pending, so that it will be draw in the
 *                                 main thread.
 *
 * @param cursor_info              The WhistCursorInfo to use for the new cursor
 *
 * @note                           ALL rendering related APIs are only safe inside main thread.
 */
void sdl_set_cursor_info_as_pending(WhistCursorInfo* cursor_info);

/**
 * @brief                          Do the rendering job of the pending cursor info, if any.
 *
 * @note                           This function is virtually instantaneous. Should be only called
 *                                 in main thread, since it's the only safe way to do any render.
 */
void sdl_present_pending_cursor(WhistFrontend* frontend);

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
 * @brief                          The above functions may be expensive, and thus may
 *                                 have to be queued up. This function executes all of the
 *                                 currently internally queued actions.
 *
 * @param frontend                 The WhistFrontend to use for the actions
 *
 * @note                           This function must be called by the
 *                                 same thread that originally called init_sdl.
 *                                 This will also render out any pending framebuffer,
 *                                 thereby setting `sdl_render_pending` to false.
 */
void sdl_update_pending_tasks(WhistFrontend* frontend);

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

#endif  // WHIST_SDL_UTILS_H
