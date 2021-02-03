/**
 * Copyright Fractal Computers, Inc. 2021
 * @file color.c
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

#include "color.h"

/*
============================
Public Function Implementations
============================
*/

#define CLAMP_COLOR(x) (((x) < 0) ? 0 : (((x) > 255) ? 255 : (x)))

RGBColor yuv_to_rgb(YUVColor yuv_color) {
    /*
        Convert a color from YUV to RGB.

        Arguments:
            yuv_color (YUVColor): YUV color to convert to RGB

        Return:
            rgb_color (RGBColor): The converted color, or black {0} on failure.
    */

    RGBColor rgb_color = {
        (uint8_t)lround(1.164 * ((float)yuv_color.y - 16.) + 1.596 * ((float)yuv_color.v - 128.)),
        (uint8_t)lround(1.164 * ((float)yuv_color.y - 16.) - 0.392 * ((float)yuv_color.u - 128.) -
                        0.813 * ((float)yuv_color.v - 128.)),
        (uint8_t)lround(1.164 * ((float)yuv_color.y - 16.) + 2.017 * ((float)yuv_color.u - 128.))};

    rgb_color.red = CLAMP_COLOR(rgb_color.red);
    rgb_color.green = CLAMP_COLOR(rgb_color.green);
    rgb_color.blue = CLAMP_COLOR(rgb_color.blue);

    return rgb_color;
}

bool color_requires_dark_text(RGBColor rgb_color) {
    /*
        Figure out whether a given background color needs foreground text to be dark
        or light, according to https://stackoverflow.com/a/3943023.

        Arguments:
            rgb_color (RGBColor): RGB background color

        Return:
            (bool): True if the color needs dark text, and false if the color needs light text.
    */
    return ((float)rgb_color.red * 0.2126 + (float)rgb_color.green * 0.7152 +
                (float)rgb_color.blue * 0.0722 >
            255. * 0.179);
}
