/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
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

#define CLAMP_COLOR(x) (uint8_t)(((x) < 0) ? 0 : (((x) > 255) ? 255 : (x)))

FractalRGBColor yuv_to_rgb(FractalYUVColor yuv_color) {
    /*
        Convert a color from YUV to RGB.

        Arguments:
            yuv_color (FractalYUVColor): YUV color to convert to RGB

        Return:
            rgb_color (FractalRGBColor): The converted color, or black {0} on failure.
    */

    float y_delta = (float)(yuv_color.y - 16);
    float u_delta = (float)(yuv_color.u - 128);
    float v_delta = (float)(yuv_color.v - 128);

    int r_component = lround(1.164 * y_delta + 1.596 * v_delta);
    int g_component = lround(1.164 * y_delta - 0.392 * u_delta - 0.813 * v_delta);
    int b_component = lround(1.164 * y_delta + 2.017 * u_delta);

    FractalRGBColor rgb_color = {CLAMP_COLOR(r_component), CLAMP_COLOR(g_component),
                                 CLAMP_COLOR(b_component)};

    return rgb_color;
}

int rgb_compare(FractalRGBColor lhs, FractalRGBColor rhs) {
    /*
        Compare two RGB colors for equality.

        Arguments:
            lhs (FractalRGBColor): The first color for comparison
            rhs (FractalRGBColor): The second color for comparison

        Return:
            (int): 0 if the colors are equal, else 1.
    */
    if (lhs.red == rhs.red && lhs.green == rhs.green && lhs.blue == rhs.blue) {
        return 0;
    }
    return 1;
}

bool color_requires_dark_text(FractalRGBColor rgb_color) {
    /*
        Figure out whether a given background color needs foreground text to be dark
        or light, according to https://stackoverflow.com/a/3943023.

        Arguments:
            rgb_color (FractalRGBColor): RGB background color

        Return:
            (bool): True if the color needs dark text, and false if the color needs light text.
    */
    return ((float)rgb_color.red * 0.2126 + (float)rgb_color.green * 0.7152 +
                (float)rgb_color.blue * 0.0722 >
            255. * 0.179);
}
