#include "../core/fractal.h"
#include "mouse.h"

RGB_Color MOUSE_COLORS[MAX_NUM_CLIENTS] = {
    {.r = 0,   .g = 188, .b = 242}, // Process Cyan
    {.r = 236, .g = 0,   .b = 140}, // Process Magenta
    {.r = 0,   .g = 158, .b = 73},  // Green 355
    {.r = 255, .g = 241, .b = 0},   // Process Yellow
    {.r = 232, .g = 17,  .b = 35},  // Red 185
    {.r = 104, .g = 33,  .b = 122}, // Purple 526
    {.r = 0,   .g = 24,  .b = 143}, // Blue 286
    {.r = 0,   .g = 178, .b = 148}, // Teal 3275
    {.r = 255, .g = 140, .b = 0},   // Orange 144
    {.r = 186, .g = 216, .b = 10},  // Lime 382
};
