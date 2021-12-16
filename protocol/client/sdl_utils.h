#ifndef SDL_UTILS_H
#define SDL_UTILS_H
/**
 * Copyright 2021 Whist Technologies, Inc.
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

/*
============================
Public Functions
============================
*/

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
 * @param icon_filename            The filename of the window icon, pointing to a 64x64 png
 *
 * @returns                        NULL if fails to create SDL window, else it
 *                                 returns the SDL window variable
 */
SDL_Window* init_sdl(int output_width, int output_height, char* name, char* icon_filename);

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
#define SDL_TEXTURE_PIXEL_FORMAT AV_PIX_FMT_NV12
/**
 * @brief                          Update the renderer's framebuffer,
 *                                 using the provided texture.
 *
 * @param data                     The data pointers to the image
 * @param linesize                 The linesize data for the image
 * @param width                    The width of the image
 * @param height                   The height of the image
 */
void sdl_update_framebuffer(Uint8* data[4], int linesize[4], int width, int height);

/**
 * @brief                          Render the most recently updated framebuffer.
 *                                 This takes some time, <1ms normally, ~8ms if VSYNC_ON
 */
void sdl_render_framebuffer();

/**
 * @brief                          Update the cursor
 *
 * @param cursor_image             The WhistCursorImage to use for the new cursor
 *
 * @note                           This function is virtually instantaneous
 */
void sdl_update_cursor(WhistCursorImage* cursor_image);

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
bool sdl_is_window_visible();

/**
 * @brief                          The above functions may be expensive, and thus may
 *                                 have to be queued up. This function executes all of the
 *                                 currently internally queued actions.
 *
 * @note                           This function must be called by the
 *                                 same thread that originally called init_sdl
 */
void update_pending_sdl_tasks();

#endif  // SDL_UTILS_H
