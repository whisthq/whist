#ifndef PNG_H
#define PNG_H
/**
 * Copyright 2021 Whist Technologies, Inc.
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

/**
 * @brief                          Converts a given bmp into a png
 *
 * @param bmp_data                 The bmp data to convert
 * @param bmp_size                 The size of the bmp data
 * @param png_data                 *png_data will be the png data
 * @param png_size                 *png_size will be the png's size
 *
 * @returns                        0 on success, -1 on failure
 */
int bmp_to_png(char* bmp_data, int bmp_size, char** png_data, int* png_size);

/**
 * @brief                          Converts a given png into a bmp
 *
 * @param png_data                 The png data to convert
 * @param png_size                 The size of the png data
 * @param bmp_data                 *bmp_data will be the bmp data
 * @param bmp_size                 *bmp_size will be the bmp's size
 *
 * @returns                        0 on success, -1 on failure
 */
int png_to_bmp(char* png_data, int png_size, char** bmp_data, int* bmp_size);

/**
 * @brief                          Will free png_data returned by bmp_to_png
 *
 * @param png_data                 The png data to free
 */
void free_png(char* png_data);

/**
 * @brief                          Will free bmp_data returned by png_to_bmp
 *
 * @param bmp_data                 The bmp data to free
 */
void free_bmp(char* bmp_data);

#endif
