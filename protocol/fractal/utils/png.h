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

/*
============================
Public Functions
============================
*/

int bmp_to_png(char* bmp, int bmp_size, char* png, int* png_size);

int png_to_bmp(char* png, int png_size, char* bmp, int* bmp_size);

#endif
