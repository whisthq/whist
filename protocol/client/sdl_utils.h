#ifndef SDL_UTILS_H
#define SDL_UTILS_H
/**
 * Copyright Fractal Computers, Inc. 2020
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

#include <fractal/core/fractal.h>
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
 * @brief                          Event watcher to be used in SDL_AddEventWatch to capture
 *                                 and handle window resize events
 *
 * @param data                     SDL Window data -- the SDL Window passed in as a void *
 *
 * @param event                    The SDL event to be analyzed
 *
 * @returns                        0 on success (fatal exit on failure)
 */
int resizing_event_watcher(void* data, SDL_Event* event);

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

/**
 * @brief                          Wrapper around SDL_CreateMutex that will correctly exit the
 *                                 protocol when SDL_LockMutex fails
 */
SDL_mutex* safe_SDL_CreateMutex();  // NOLINT(readability-identifier-naming)

/**
 * @brief                          Wrapper around SDL_LockMutex that will correctly exit the
 *                                 protocol when SDL_LockMutex fails
 */
void safe_SDL_LockMutex(SDL_mutex* mutex);  // NOLINT(readability-identifier-naming)

/**
 * @brief                          Wrapper around SDL_TryLockMutex that will correctly exit the
 *                                 protocol when SDL_TryLockMutex fails
 */
int safe_SDL_TryLockMutex(SDL_mutex* mutex);  // NOLINT(readability-identifier-naming)

/**
 * @brief                          Wrapper around SDL_UnlockMutex that will correctly exit the
 * protocol when SDL_UnlockMutex fails
 */
void safe_SDL_UnlockMutex(SDL_mutex* mutex);  // NOLINT(readability-identifier-naming)

/**
 * @brief                          Wrapper around SDL_CondWait that will correctly exit the
 *                                 protocol when SDL_LockMutex fails
 */
void safe_SDL_CondWait(SDL_cond* cond, SDL_mutex* mutex);  // NOLINT(readability-identifier-naming)

#endif  // SDL_UTILS_H
