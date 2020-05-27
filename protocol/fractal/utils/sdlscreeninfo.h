#ifndef SDL_SCREEN_INFO_H
#define SDL_SCREEN_INFO_H
/**

@brief This file contains SDL screen info code. (Requires additional documenting)

============================
Usage
============================


*/


#include "../core/fractal.h"

int get_window_pixel_width(SDL_Window* window);
int get_window_pixel_height(SDL_Window* window);
int get_window_virtual_width(SDL_Window* window);
int get_window_virtual_height(SDL_Window* window);
int get_virtual_screen_width();
int get_virtual_screen_height();
int get_pixel_screen_width(SDL_Window* window);
int get_pixel_screen_height(SDL_Window* window);

#endif  // SDL_SCREEN_INFO_H