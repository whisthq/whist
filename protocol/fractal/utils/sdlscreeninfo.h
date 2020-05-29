#ifndef SDL_SCREEN_INFO_H
#define SDL_SCREEN_INFO_H
/**
 * @file sdlscreeninfo.h
 * @brief This file contains SDL screen info code, used to retrive information
 *        about SDL Windows.
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
 * @param window		           SDL Window to find the physical width of, in
 *                                 pixels
 *
 * @returns                        The width of the SDL Window, in pixels
 */
int get_window_pixel_width(SDL_Window* window);

/**
 * @brief                          Get the physical height of an SDL Window, in
 *                                 pixels
 *
 * @param window		           SDL Window to find the physical height of, in
 *                                 pixels
 *
 * @returns                        The physical height of the SDL Window, in
 *                                 pixels
 */
int get_window_pixel_height(SDL_Window* window);

/**
 * @brief                          Get the virtual width of an SDL Window, in
 *                                 pixels (i.e. 1440x900 instead of 2880x1800)
 *
 * @param window		           SDL Window to find the virtual width of, in
 *                                 pixels
 *
 * @returns                        The virtual width of the SDL Window, in
 *                                 pixels
 */
int get_window_virtual_width(SDL_Window* window);

/**
 * @brief                          Get the virtual height of an SDL Window, in
 *                                 pixels (i.e. 1440x900 instead of 2880x1800)
 *
 * @param window		           SDL Window to find the virtual height of, in
 *                                 pixels
 *
 * @returns                        The virtual height of the SDL Window, in
 *                                 pixels
 */
int get_window_virtual_height(SDL_Window* window);

/**
 * @brief                          Get the virtual width of a monitor/screen, in
 *                                 pixels (i.e. 1440x900 instead of 2880x1800)
 *
 * @returns                        The virtual width of the monitor/screen, in
 *                                 pixels
 */
int get_virtual_screen_width();

/**
 * @brief                          Get the virtual height of a monitor/screen,
 *                                 in pixels (i.e. 1440x900 instead of
 * 2880x1800)
 *
 * @returns                        The virtual height of the monitor/screen, in
 *                                 pixels
 */
int get_virtual_screen_height();

/**
 * @brief                          Get the physical width of a monitor/screen,
 *                                 in pixels
 *
 * @param window		           SDL Window associated with the monitor/screen
 *                                 to find the physical width of, in pixels
 *
 * @returns                        The physical width of the monitor/screen, in
 *                                 pixels
 */
int get_pixel_screen_width(SDL_Window* window);

/**
 * @brief                          Get the physical height of a monitor/screen,
 *                                 in pixels
 *
 * @param window		           SDL Window associated with the monitor/screen
 *                                 to find the physical height of, in pixels
 *
 * @returns                        The physical height of the monitor/screen, in
 *                                 pixels
 */
int get_pixel_screen_height(SDL_Window* window);

#endif  // SDL_SCREEN_INFO_H