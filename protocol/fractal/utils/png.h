#ifndef PNG_H
#define PNG_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clock.h
 * @brief Helper functions for png to bmp and bmp to png
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <libavcodec/avcodec.h>
#include <SDL2/SDL.h>

/*
============================
Public Functions
============================
*/

int bmp_to_png(unsigned char* bmp, unsigned int size, AVPacket* pkt);

int png_char_to_bmp(char* png, int size, AVPacket* pkt);

/**
 * @brief                          Load a PNG file to an SDL surface using lodepng.
 *
 * @param filename                 PNG image file path
 *
 * @returns                        the loaded surface on success, and NULL on failure
 *
 * @note                           After a successful call to sdl_surface_from_png_file,
 *                                 remember to call `SDL_FreeSurface(surface)` to free memory.
 */
SDL_Surface* sdl_surface_from_png_file(char* filename);

#endif
