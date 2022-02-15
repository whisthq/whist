#ifndef CURSOR_H
#define CURSOR_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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
#include <sys/types.h>

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
typedef enum WhistCursorState {
    CURSOR_STATE_HIDDEN = 0,
    CURSOR_STATE_VISIBLE = 1
} WhistCursorState;

/**
 * @brief   Cursor ID.
 * @details The type of the cursor showing up on the screen.
 */
typedef enum WhistCursorID {
    WHIST_CURSOR_ARROW,
    WHIST_CURSOR_IBEAM,
    WHIST_CURSOR_WAIT,
    WHIST_CURSOR_CROSSHAIR,
    WHIST_CURSOR_WAITARROW,
    WHIST_CURSOR_SIZENWSE,
    WHIST_CURSOR_SIZENESW,
    WHIST_CURSOR_SIZEWE,
    WHIST_CURSOR_SIZENS,
    WHIST_CURSOR_SIZEALL,
    WHIST_CURSOR_NO,
    WHIST_CURSOR_HAND,
    INVALID
} WhistCursorID;

/**
 * @brief   Cursor image.
 * @details The image used for the rendered cursor.
 */
typedef struct WhistCursorInfo {
    WhistCursorID cursor_id;
    WhistCursorState cursor_state;
    uint32_t hash;
    bool using_png;
    size_t png_size;
    unsigned short png_width;
    unsigned short png_height;
    unsigned short png_hot_x;
    unsigned short png_hot_y;
    unsigned char png[];
} WhistCursorInfo;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize all cursors
 */
void init_cursors(void);

/**
 * @brief                          Returns the current cursor image
 *
 * @returns                       The current cursor image
 */
WhistCursorInfo* get_current_cursor(void);

/**
 * @brief                          Returns the size of the WhistCursorInfo struct
 *
 * @param image                    The WhistCursorInfo struct
 *
 * @returns                       The size of the WhistCursorInfo struct
 */
size_t get_cursor_info_size(WhistCursorInfo* image);

#endif  // CURSOR_H
