#ifndef CURSOR_H
#define CURSOR_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
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
#include <stdint.h>

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

/**
 * @brief   Cursor state.
 * @details State of the cursor on the rendered screen.
 */
typedef enum FractalCursorState {
    CURSOR_STATE_HIDDEN = 0,
    CURSOR_STATE_VISIBLE = 1
} FractalCursorState;

/**
 * @brief   Cursor ID.
 * @details The type of the cursor showing up on the screen.
 */
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
    FRACTAL_CURSOR_HAND,
    INVALID
} FractalCursorID;

/**
 * @brief   Cursor image.
 * @details The image used for the rendered cursor.
 */
typedef struct FractalCursorImage {
    FractalCursorID cursor_id;
    FractalCursorState cursor_state;
    bool using_bmp;
    unsigned short bmp_width;
    unsigned short bmp_height;
    unsigned short bmp_hot_x;
    unsigned short bmp_hot_y;
    uint32_t bmp[MAX_CURSOR_WIDTH * MAX_CURSOR_HEIGHT];
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
 * @param image                    FractalCursorImage buffer to write to
 */
void get_current_cursor(FractalCursorImage* image);

#endif  // CURSOR_H
