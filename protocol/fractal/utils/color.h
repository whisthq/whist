#ifndef COLOR_H
#define COLOR_H

/**
 * Copyright Fractal Computers, Inc. 2021
 * @file color.h
 * @brief This file contains functions relating to colors, colorspace conversion,
          and implementations of algorithms and heuristics related to color and UX.

============================
Usage
============================

The conversion functions take an initialized XXXColor struct and return a YYYColor struct.
Meanwhile, other functions will appropriately parse the struct and return a useful computation
from the color.

*/

/*
============================
Includes
============================
*/
#include "../core/fractal.h"

/*
============================
Custom Types
============================
*/

/// @brief RGB Color.
/// @details Red, green, and blue are in [0,255].
typedef struct RGBColor {
    uint8_t red;    //<<< Red component
    uint8_t green;  //<<< Green component
    uint8_t blue;   //<<< Blue component
} RGBColor;

/// @brief YUV Color.
/// @details Y', U, and V are in [0,255], and correspond to the values in
///          YUV420p data. Y' is luma, U is blue-projected chrominance, and
///          V is red-projected chrominance.
typedef struct YUVColor {
    uint8_t y;  //<<< Y component
    uint8_t u;  //<<< U component
    uint8_t v;  //<<< V component
} YUVColor;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Convert from YUV to RGB.
 *
 * @param yuv_color                The YUV color to convert to RGB.
 *
 * @returns                        A filled struct with the converted color, or black {0} on
 * failure.
 */
RGBColor yuv_to_rgb(YUVColor yuv_color);

/**
 * @brief                          Figure out whether a given background color needs foreground text
 * to be dark or light, according to https://stackoverflow.com/a/3943023
 *
 * @param rgb_color                The background color.
 *
 * @returns                        True if the color needs dark text, and false if the color needs
 * light text.
 */
bool color_requires_dark_text(RGBColor rgb_color);

#endif  // COLOR_H