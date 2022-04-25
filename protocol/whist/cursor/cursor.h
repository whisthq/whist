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
 * @brief   Mouse mode.
 * @details The mouse mode -- normal or relative. Relative
 *          occurs when the cursor is locked/captured by
 *          an application.
 */
typedef enum WhistMouseMode {
    MOUSE_MODE_NORMAL = 0,
    MOUSE_MODE_RELATIVE = 1,
} WhistMouseMode;

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
    WhistMouseMode mode;
    uint32_t hash;
    bool cached;
    size_t png_size;
    unsigned short png_width;
    unsigned short png_height;
    unsigned short png_hot_x;
    unsigned short png_hot_y;
    unsigned char png[];
} WhistCursorInfo;

typedef struct WhistCursorCache WhistCursorCache;

/*
============================
Public Functions
============================
*/

typedef struct WhistCursorManager WhistCursorManager;

/** @brief */
typedef enum {
	WHIST_CURSOR_X11,
	WHIST_CURSOR_WESTON,
	WHIST_CURSOR_WIN32
} WhistCursorKind;

typedef WhistCursorInfo* (*whist_cursor_capture_cb)(WhistCursorManager* cursor);
typedef void (*whist_cursor_uninit_cb)(WhistCursorManager* cursor);
typedef WhistMouseMode (*whist_cursor_get_latest_mouse_mode_cb)(WhistCursorManager* cursor);


/** @brief */
struct WhistCursorManager {
	WhistCursorKind kind;

	whist_cursor_capture_cb capture;
	whist_cursor_get_latest_mouse_mode_cb get_latest_mouse_mode;
	whist_cursor_uninit_cb uninit;
};


/**
 * @brief                          Initialize cursor capturing
 */
WhistCursorManager* whist_cursor_capture_init(WhistCursorKind kind, void *args);

/**
 * @brief                          Shut down cursor capturing
 */
void whist_cursor_capture_destroy(WhistCursorManager* cursor);

/**
 * @brief                          Returns the size of the WhistCursorInfo struct
 *
 * @param image                    The WhistCursorInfo struct
 *
 * @returns                        The size of the WhistCursorInfo struct
 */
size_t whist_cursor_info_get_size(const WhistCursorInfo* image);

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
//WhistCursorInfo* whist_cursor_capture(void);



/**
 * @brief                          Convert a cursor type to a string name
 *
 * @param type                     The cursor type to convert
 *
 * @returns                        The string name of the cursor type
 */
const char* whist_cursor_type_to_string(WhistCursorType type);

/**
 * Create a new cursor cache instance.
 *
 * @param max_entries  The number of entries in the cache.
 * @param store_data   Whether to store full cursor data (metadata is
 *                     always stored).
 * @return  New cursor cache instance.
 */
WhistCursorCache* whist_cursor_cache_create(int max_entries, bool store_data);

/**
 * Clear the cursor cache of all entries.
 *
 * @param cache  Cache instance to clear.
 */
void whist_cursor_cache_clear(WhistCursorCache* cache);

/**
 * Destroy a cursor cache instance.
 *
 * @param cache  Cache instance to destroy.
 */
void whist_cursor_cache_destroy(WhistCursorCache* cache);

/**
 * Add a new entry to the cursor cache.
 *
 * If the cache is already full this will evict the least recently used
 * entry currently in the cache.
 *
 * @param cache  Cache to add new entry to.
 * @param info   Cursor to add to the cache.  All data is copied.
 */
void whist_cursor_cache_add(WhistCursorCache* cache, WhistCursorInfo* info);

/**
 * Check whether a given hash is in the cache.
 *
 * @param cache  Cache to do the lookup in.
 * @param hash   Hash to look for.
 * @return  Cursor with the matching hash, or NULL if not found.
 */
const WhistCursorInfo* whist_cursor_cache_check(WhistCursorCache* cache, uint32_t hash);

#endif  // CURSOR_H
