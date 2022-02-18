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
#include <whist/utils/aes.h>

#include "cursor.h"

size_t whist_cursor_info_get_size(WhistCursorInfo* cursor_info) {
    FATAL_ASSERT(cursor_info != NULL);

    if (cursor_info->using_png) {
        return sizeof(WhistCursorInfo) + cursor_info->png_size;
    } else {
        return sizeof(WhistCursorInfo);
    }
}

WhistCursorInfo* whist_cursor_info_from_id(WhistCursorID id, WhistCursorState state) {
    WhistCursorInfo* info = safe_malloc(sizeof(WhistCursorInfo));
    memset(info, 0, sizeof(WhistCursorInfo));
    info->using_png = false;
    info->cursor_id = id;
    info->cursor_state = state;
    info->hash = hash(&id, sizeof(WhistCursorID));
    return info;
}

WhistCursorInfo* whist_cursor_info_from_rgba(const uint32_t* rgba, unsigned short width,
                                             unsigned short height, unsigned short hot_x,
                                             unsigned short hot_y, WhistCursorState state) {
    unsigned char* png;
    size_t png_size;

    unsigned int ret = lodepng_encode32(&png, &png_size, (unsigned char*)&rgba, width, height);
    if (ret) {
        LOG_WARNING("Failed to encode PNG cursor: %s", lodepng_error_text(ret));
        return NULL;
    }

    WhistCursorInfo* info = safe_malloc(sizeof(WhistCursorInfo) + png_size);
    info->using_png = true;
    info->png_width = width;
    info->png_height = height;
    info->png_size = png_size;
    info->png_hot_x = hot_x;
    info->png_hot_y = hot_y;
    info->cursor_state = state;
    memcpy(info->png, png, png_size);
    info->hash = hash(png, png_size);
    free(png);
    return info;
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
