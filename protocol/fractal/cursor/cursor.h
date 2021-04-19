#ifndef CURSOR_H
#define CURSOR_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file cursor.h
 * @brief This file defines the cursor types, functions, init and get.
============================
Usage
============================

Use InitCursor to load the appropriate cursor images for a specific OS, and then
GetCurrentCursor to retrieve what the cursor shold be on the OS (drag-window,
arrow, etc.).
*/

/*
============================
Includes
============================
*/

#include <stdbool.h>

/*
============================
Defines
============================
*/

#define MAX_CURSOR_WIDTH 64
#define MAX_CURSOR_HEIGHT 64

/*
============================
Custom Types
============================
*/

typedef enum FractalCursorState {
    CURSOR_STATE_HIDDEN = 0,
    CURSOR_STATE_VISIBLE = 1
} FractalCursorState;

typedef enum FractalCursorID {
    FRACTAL_CURSOR_ARROW,
    FRACTAL_CURSOR_IBEAM,
    FRACTAL_CURSOR_WAIT,
    FRACTAL_CURSOR_CROSSHAIR,
    FRACTAL_CURSOR_WAITARROW,
    FRACTAL_CURSOR_SIZENWSE,
    FRACTAL_CURSOR_SIZENESW,
    FRACTAL_CURSOR_SIZEWE,
    FRACTAL_CURSOR_SIZENS,
    FRACTAL_CURSOR_SIZEALL,
    FRACTAL_CURSOR_NO,
    FRACTAL_CURSOR_HAND
} FractalCursorID;

typedef struct FractalCursorImage {
    FractalCursorID cursor_id;
    FractalCursorState cursor_state;
    bool cursor_use_bmp;
    unsigned short cursor_bmp_width;
    unsigned short cursor_bmp_height;
    unsigned short cursor_bmp_hot_x;
    unsigned short cursor_bmp_hot_y;
    uint32_t cursor_bmp[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT];
} FractalCursorImage;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize all cursors
 */
void init_cursors();

/**
 * @brief                          Returns the current cursor image
 *
 * @returns                        Current FractalCursorImage
 */
FractalCursorImage get_current_cursor();

#endif  // CURSOR_H
