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

============================
Testing
============================

The functionality provided by this file is tested in `test/ClientTest.cpp`, in `FractalColorTest`.
*/

/*
============================
Includes
============================
*/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/*
============================
Custom Types
============================
*/

/// @brief RGB Color.
/// @details Red, green, and blue are in [0,255].
typedef struct FractalRGBColor {
    uint8_t red;    //<<< Red component
    uint8_t green;  //<<< Green component
    uint8_t blue;   //<<< Blue component
} FractalRGBColor;

/// @brief YUV Color.
/// @details Y', U, and V are in [0,255], and correspond to the values in
///          YUV420p data. Y' is luma, U is blue-projected chrominance, and
///          V is red-projected chrominance.
typedef struct FractalYUVColor {
    uint8_t y;  //<<< Y component
    uint8_t u;  //<<< U component
    uint8_t v;  //<<< V component
} FractalYUVColor;

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
 *                                 failure.
 */
FractalRGBColor yuv_to_rgb(FractalYUVColor yuv_color);

/**
 * @brief                          Compare two RGB colors for equality
 *
 * @param lhs                      The first color for comparison
 *
 * @param rhs                      The second color for comparison
 *
 * @returns                        0 if the colors are equal, else 1
 */
int rgb_compare(FractalRGBColor lhs, FractalRGBColor rhs);

/**
 * @brief                          Figure out whether a given background color needs foreground text
 *                                 to be dark or light, according to
 *                                 https://stackoverflow.com/a/3943023
 *
 * @param rgb_color                The background color.
 *
 * @returns                        True if the color needs dark text, and false if the color needs
 *                                 light text.
 */
bool color_requires_dark_text(FractalRGBColor rgb_color);

#endif  // COLOR_H
