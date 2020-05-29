#ifndef SDL_UTILS_H
#define SDL_UTILS_H
/**
 * @file sdl_utils.h
 * @brief This file contains the code to create and destroy SDL windows on the client.
============================
Usage
============================

initSDL gets called first to create an SDL window, and destroySDL at the end to close the window.
*/

/*
============================
Includes
============================
*/

#include "../fractal/core/fractal.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "video.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Attaches the current thread to the specified current input desktop
 * 
 * @param output_width             The width of the SDL window to create, in pixels
 * @param output_height            The height of the SDL window to create, in pixels
 * 
 * @returns                        NULL if fails to create SDL window, else it returns the SDL window variable
 */
SDL_Window* initSDL(int output_width, int output_height);

/**
 * @brief                          Destroys an SDL window and associated parameters 
 *
 * @param window                   The SDL window to destroy        
 */
void destroySDL(SDL_Window* window);

#endif  // SDL_UTILS_H
