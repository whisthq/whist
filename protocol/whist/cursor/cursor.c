/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file cursor.c
 * @brief This file defines platform-independent cursor streaming utilities.
============================
Usage
============================

Use whist_cursor_info_get_size() to get the size of the cursor info struct,
including a potential embedded PNG.
*/

#include <whist/core/whist.h>
#include <whist/utils/lodepng.h>

#include "cursor.h"

size_t whist_cursor_info_get_size(WhistCursorInfo* cursor_info) {
    FATAL_ASSERT(cursor_info != NULL);

    if (cursor_info->type == WHIST_CURSOR_PNG) {
        return sizeof(WhistCursorInfo) + cursor_info->png_size;
    } else {
        return sizeof(WhistCursorInfo);
    }
}

uint8_t* whist_cursor_info_to_rgba(const WhistCursorInfo* info) {
    FATAL_ASSERT(info != NULL);
    FATAL_ASSERT(info->type == WHIST_CURSOR_PNG);

    uint8_t* rgba;
    unsigned int width, height;

    unsigned int ret = lodepng_decode32(&rgba, &width, &height, info->png, info->png_size);
    if (ret) {
        LOG_ERROR("Failed to decode cursor png: %s", lodepng_error_text(ret));
        return NULL;
    }

    // Sanity check to ensure our struct matches the decoded image
    FATAL_ASSERT(width == info->png_width);
    FATAL_ASSERT(height == info->png_height);

    return rgba;
}

const char* whist_cursor_type_to_string(WhistCursorType type) {
    static const char* map[] = {
        "none",         "png",        "alias",          "all-scroll",  "arrow",      "cell",
        "context-menu", "copy",       "crosshair",      "grab",        "grabbing",   "hand",
        "help",         "ibeam",      "ibeam-vertical", "move",        "no-drop",    "not-allowed",
        "progress",     "resize-col", "resize-e",       "resize-ew",   "resize-n",   "resize-ne",
        "resize-nesw",  "resize-ns",  "resize-nw",      "resize-nwse", "resize-row", "resize-s",
        "resize-se",    "resize-sw",  "resize-w",       "wait",        "zoom-in",    "zoom-out",
    };
    return map[type];
}
