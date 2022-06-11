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

size_t whist_cursor_info_get_size(const WhistCursorInfo* cursor_info) {
    FATAL_ASSERT(cursor_info != NULL);

    if (cursor_info->using_png) {
        return sizeof(WhistCursorInfo) + cursor_info->png_size;
    } else {
        return sizeof(WhistCursorInfo);
    }
}

WhistCursorInfo* whist_cursor_dup(const WhistCursorInfo* cursor) {
    size_t size = whist_cursor_info_get_size(cursor);
    WhistCursorInfo* new = safe_malloc(size);
    memcpy(new, cursor, sizeof(*new));
    if (new->using_png) {
        new->png_data = (uint8_t*)(new + 1);
        memcpy(new->png_data, cursor->png_data, new->png_size);
    } else {
        new->png_data = NULL;
    }
    return new;
}

uint8_t* whist_cursor_info_to_rgba(const WhistCursorInfo* info) {
    FATAL_ASSERT(info != NULL);
    FATAL_ASSERT(info->using_png);

    uint8_t* rgba;
    unsigned int width, height;

    unsigned int ret = lodepng_decode32(&rgba, &width, &height, info->png_data, info->png_size);
    if (ret) {
        LOG_ERROR("Failed to decode cursor png: %s", lodepng_error_text(ret));
        return NULL;
    }

    // Sanity check to ensure our struct matches the decoded image
    FATAL_ASSERT(width == info->png_width);
    FATAL_ASSERT(height == info->png_height);

    return rgba;
}

typedef struct WhistCursorCacheEntry {
    LINKED_LIST_HEADER;
    WhistCursorInfo* info;
    WhistTimer last_touch;
} WhistCursorCacheEntry;

struct WhistCursorCache {
    LinkedList list;
    int max_entries;
    bool store_data;
};

WhistCursorCache* whist_cursor_cache_create(int max_entries, bool store_data) {
    WhistCursorCache* cache = safe_zalloc(sizeof(*cache));
    cache->max_entries = max_entries;
    cache->store_data = store_data;
    return cache;
}

static void whist_cursor_cache_free_entry(WhistCursorCacheEntry* entry) {
    free(entry->info);
    free(entry);
}

void whist_cursor_cache_clear(WhistCursorCache* cache) {
    WhistCursorCacheEntry* entry;
    while ((entry = linked_list_extract_head(&cache->list))) {
        whist_cursor_cache_free_entry(entry);
    }
}

void whist_cursor_cache_destroy(WhistCursorCache* cache) {
    whist_cursor_cache_clear(cache);
    free(cache);
}

void whist_cursor_cache_add(WhistCursorCache* cache, WhistCursorInfo* info) {
    if (linked_list_size(&cache->list) == cache->max_entries) {
        // Remove the oldest entry.
        WhistCursorCacheEntry* oldest = linked_list_extract_tail(&cache->list);
        whist_cursor_cache_free_entry(oldest);
    }

    // Add the new entry at the front of the list.
    WhistCursorCacheEntry* entry = safe_zalloc(sizeof(*entry));
    if (cache->store_data) {
        entry->info = whist_cursor_dup(info);
    } else {
        entry->info = safe_malloc(sizeof(*info));
        memcpy(entry->info, info, sizeof(*info));
        entry->info->cached = true;
        entry->info->png_size = 0;
    }
    start_timer(&entry->last_touch);
    linked_list_add_head(&cache->list, entry);
}

const WhistCursorInfo* whist_cursor_cache_check(WhistCursorCache* cache, uint32_t hash) {
    linked_list_for_each(&cache->list, WhistCursorCacheEntry, entry) {
        if (entry->info->hash == hash) {
            // We matched, so move this entry to the front of the list.
            linked_list_remove(&cache->list, entry);
            linked_list_add_head(&cache->list, entry);
            start_timer(&entry->last_touch);
            return entry->info;
        }
    }
    // Not found.
    return NULL;
}
