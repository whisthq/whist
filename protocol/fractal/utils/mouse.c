#include <fractal/core/fractal.h>
#include "mouse.h"

FractalRGBColor mouse_colors[MAX_NUM_CLIENTS] = {
    {.red = 0, .green = 188, .blue = 242},   // Process Cyan
    {.red = 236, .green = 0, .blue = 140},   // Process Magenta
    {.red = 0, .green = 158, .blue = 73},    // Green 355
    {.red = 255, .green = 241, .blue = 0},   // Process Yellow
    {.red = 232, .green = 17, .blue = 35},   // Red 185
    {.red = 104, .green = 33, .blue = 122},  // Purple 526
    {.red = 0, .green = 24, .blue = 143},    // Blue 286
    {.red = 0, .green = 178, .blue = 148},   // Teal 3275
    {.red = 255, .green = 140, .blue = 0},   // Orange 144
    {.red = 186, .green = 216, .blue = 10},  // Lime 382
};
