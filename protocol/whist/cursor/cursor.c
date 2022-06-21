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

    if (cursor_info->using_png) {
        return sizeof(WhistCursorInfo) + cursor_info->png_size;
    } else {
        return sizeof(WhistCursorInfo);
    }
}

uint8_t* whist_cursor_info_to_rgba(const WhistCursorInfo* info) {
    FATAL_ASSERT(info != NULL);
    FATAL_ASSERT(info->using_png);

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
