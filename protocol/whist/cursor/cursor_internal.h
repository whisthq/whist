#ifndef WHIST_CURSOR_INTERNAL
#define WHIST_CURSOR_INTERNAL
/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file cursor_internal.h
 * @brief This file defines platform-independent internal utilities for cursor capture
============================
Usage
============================
Our various cursor capture implementations use these functions to generate `WhistCursorInfo`
structs.
*/

/*
============================
Includes
============================
*/

#include "cursor.h"

/*
============================
Defines
============================
*/

/**
 * @brief                          Returns WhistCursorInfo from a cursor id
 *
 * @param id                       The WhistCursorID from which to generate the cursor info
 * @param state                    The cursor visibility state (visible or hidden)
 *
 * @returns                        The generated cursor info as a pointer to
 *                                 a WhistCursorInfo struct, which must be freed
 *                                 by the caller
 */
WhistCursorInfo* whist_cursor_info_from_id(WhistCursorID id, WhistCursorState state);

/**
 * @brief                          Returns WhistCursorInfo from RGBA pixel data
 *
 * @param rgba                     The RGBA pixel data from which to generate the cursor info as a
 *                                 uint32_t array
 *
 * @param width                    The width of the RGBA pixel data
 * @param height                   The height of the RGBA pixel data
 * @param hot_x                    The x-coordinate of the cursor hotspot
 * @param hot_y                    The y-coordinate of the cursor hotspot
 * @param state                    The cursor visibility state (visible or hidden)
 *
 * @returns                        The generated cursor info as a pointer to
 *                                 a WhistCursorInfo struct, which must be freed
 *                                 by the caller
 */
WhistCursorInfo* whist_cursor_info_from_rgba(const uint32_t* rgba, unsigned short width,
                                             unsigned short height, unsigned short hot_x,
                                             unsigned short hot_y, WhistCursorState state);
#endif  // WHIST_CURSOR_INTERNAL
