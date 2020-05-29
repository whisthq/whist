#ifndef SDL_SCREEN_INFO_H
#define SDL_SCREEN_INFO_H
/**
 * @file sdlscreeninfo.h
 * @brief This file contains SDL screen info code.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Get the width of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The width of the SDL Window, in pixels
*/
int get_window_pixel_width(SDL_Window* window);

/**
 * @brief                          Get the physical height of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The physical height of the SDL Window, in pixels
*/
int get_window_pixel_height(SDL_Window* window);

/**
 * @brief                          Get the virtual width of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The virtual width of the SDL Window, in pixels
*/
int get_window_virtual_width(SDL_Window* window);

/**
 * @brief                          Get the virtual height of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The virtual height of the SDL Window, in pixels
*/
int get_window_virtual_height(SDL_Window* window);





/**
 * @brief                          Get the width of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The width of the SDL Window, in pixels
*/
int get_virtual_screen_width();

/**
 * @brief                          Get the width of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The width of the SDL Window, in pixels
*/
int get_virtual_screen_height();

/**
 * @brief                          Get the width of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The width of the SDL Window, in pixels
*/
int get_pixel_screen_width(SDL_Window* window);

/**
 * @brief                          Get the width of an SDL Window, in pixels
 *
 * @param window		           SDL Window to find the width of, in pixels
 * 
 * @returns                        The width of the SDL Window, in pixels
*/
int get_pixel_screen_height(SDL_Window* window);






#endif  // SDL_SCREEN_INFO_H