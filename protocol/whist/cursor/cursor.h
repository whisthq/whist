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
#include <stddef.h>

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
typedef enum WhistCursorCaptureState {
    CURSOR_CAPTURE_STATE_NORMAL = 0,
    CURSOR_CAPTURE_STATE_CAPTURED = 1,
} WhistCursorCaptureState;

/**
 * @brief   Cursor type.
 * @details The type of cursor -- either no cursor, a PNG image, or a system cursor type.
 */
typedef enum WhistCursorType {
    WHIST_CURSOR_NONE,
    WHIST_CURSOR_PNG,
    WHIST_CURSOR_ALIAS,
    WHIST_CURSOR_ALL_SCROLL,
    WHIST_CURSOR_ARROW,
    WHIST_CURSOR_CELL,
    WHIST_CURSOR_CONTEXT_MENU,
    WHIST_CURSOR_COPY,
    WHIST_CURSOR_CROSSHAIR,
    WHIST_CURSOR_GRAB,
    WHIST_CURSOR_GRABBING,
    WHIST_CURSOR_HAND,
    WHIST_CURSOR_HELP,
    WHIST_CURSOR_IBEAM,
    WHIST_CURSOR_IBEAM_VERTICAL,
    WHIST_CURSOR_MOVE,
    WHIST_CURSOR_NO_DROP,
    WHIST_CURSOR_NOT_ALLOWED,
    WHIST_CURSOR_PROGRESS,
    WHIST_CURSOR_RESIZE_COL,
    WHIST_CURSOR_RESIZE_E,
    WHIST_CURSOR_RESIZE_EW,
    WHIST_CURSOR_RESIZE_N,
    WHIST_CURSOR_RESIZE_NE,
    WHIST_CURSOR_RESIZE_NESW,
    WHIST_CURSOR_RESIZE_NS,
    WHIST_CURSOR_RESIZE_NW,
    WHIST_CURSOR_RESIZE_NWSE,
    WHIST_CURSOR_RESIZE_ROW,
    WHIST_CURSOR_RESIZE_S,
    WHIST_CURSOR_RESIZE_SE,
    WHIST_CURSOR_RESIZE_SW,
    WHIST_CURSOR_RESIZE_W,
    WHIST_CURSOR_WAIT,
    WHIST_CURSOR_ZOOM_IN,
    WHIST_CURSOR_ZOOM_OUT,
} WhistCursorType;

/**
 * @brief   Cursor image.
 * @details The image used for the rendered cursor.
 */
typedef struct WhistCursorInfo {
    WhistCursorType type;
    WhistCursorCaptureState capture_state;
    uint32_t hash;
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
 * @brief                          Initialize cursor capturing
 */
void whist_cursor_capture_init(void);

/**
 * @brief                          Shut down cursor capturing
 */
void whist_cursor_capture_destroy(void);

/**
 * @brief                          Returns the size of the WhistCursorInfo struct
 *
 * @param image                    The WhistCursorInfo struct
 *
 * @returns                        The size of the WhistCursorInfo struct
 */
size_t whist_cursor_info_get_size(WhistCursorInfo* image);

/**
 * @brief                          Return RGBA pixel data from a WhistCursorInfo struct
 *
 * @param info                     The WhistCursorInfo struct from which to generate the RGBA pixel
 * data
 *
 * @returns                        The RGBA pixel data as a pointer to a uint8_t array, which must
 * be freed by the caller
 */
uint8_t* whist_cursor_info_to_rgba(const WhistCursorInfo* info);

/**
 * @brief                          Returns the current cursor image
 *
 * @returns                        The current cursor image as a pointer to a
 *                                 WhistCursorInfo struct, which must be freed
 *                                 by the caller.
 */
WhistCursorInfo* whist_cursor_capture(void);

/**
 * @brief                          Convert a cursor type to a string name
 *
 * @param type                     The cursor type to convert
 *
 * @returns                        The string name of the cursor type
 */
const char* whist_cursor_type_to_string(WhistCursorType type);

#endif  // CURSOR_H
