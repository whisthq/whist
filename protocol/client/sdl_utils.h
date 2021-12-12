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
 * @brief                          Creates the SDL Renderer,
 *                                 using the current SDL `window`.
 *
 * @param sdl_window               The SDL Window to use
 */
SDL_Renderer* init_renderer(SDL_Window* sdl_window);

/**
 * @brief                          Destroys an SDL window and associated
 *                                 parameters
 *
 * @param window                   The SDL window to destroy
 */
void destroy_sdl(SDL_Window* window);

/**
 * @brief                          Load a PNG file to an SDL surface using lodepng.
 *
 * @param filename                 PNG image file path
 *
 * @returns                        The loaded surface on success, and NULL on failure
 *
 * @note                           After a successful call to sdl_surface_from_png_file,
 *                                 remember to call `free_sdl_rgb_surface` when you're done using
 * it!
 */
SDL_Surface* sdl_surface_from_png_file(char* filename);

/**
 * @brief                          Free the SDL_Surface and its pixels array.
 *                                 Only call this if the pixels array has been malloc'ed,
 *                                 and the surface was created via `SDL_CreateRGBSurfaceFrom`
 *
 * @param surface                  The SDL_Surface to be freed
 */
void free_sdl_rgb_surface(SDL_Surface* surface);

void sdl_render_loading_screen(SDL_Renderer* renderer, int idx);

void sdl_render_cursor(WhistCursorImage* cursor_image);

void sdl_blank_screen(SDL_Renderer* renderer);

// The pixel format required for the data/linesize passed into sdl_render_frame
#define SDL_TEXTURE_PIXEL_FORMAT AV_PIX_FMT_NV12
// Update the renderer's framebuffer
void sdl_update_framebuffer(SDL_Renderer* renderer, Uint8* data[4], int linesize[4], int width,
                            int height, int output_width, int output_height);

// Render out the frame
void sdl_render(SDL_Renderer* renderer);

void sdl_render_window_titlebar_color(WhistRGBColor color);

void sdl_set_fullscreen(bool is_fullscreen);

void sdl_set_window_title(const char* window_title);

// Call from main thread
void update_pending_sdl_tasks();

#endif  // SDL_UTILS_H
