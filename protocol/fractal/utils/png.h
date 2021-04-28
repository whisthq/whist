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

/*
============================
Public Functions
============================
*/

int bmp_to_png(char* bmp_data, int bmp_size, char** png_data, int* png_size);

/**
 * @brief                          Converts a given png into a bmp
 * 
 * @param png_data                 The png data
 * @param png_size                 The size of the png data
 * @param bmp_data                 *bmp_data will be set to the bmp data
 * @param bmp_size                 *bmp_size will be set to the bmp's size
 * @returns 
 */
int png_to_bmp(char* png_data, int png_size, char** bmp_data, int* bmp_size);

void free_png(char* png_data);
void free_bmp(char* bmp_data);

#endif
