/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file cursor.c
 * @brief This file defines platform-independent cursor streaming utilities.
 */

#include <whist/core/whist.h>
extern "C" {
// Load lodepng with C linkage
#define LODEPNG_NO_COMPILE_CPP
#include <whist/utils/lodepng.h>
#include "cursor.h"
#include "x11_cursor.h"
}
#include <deque>

typedef struct WhistCursorCacheEntry {
    WhistCursorInfo* cursor_info;
    int most_recent_cache_access_index;
} WhistCursorCacheEntry;

struct WhistCursorCache {
    std::map<uint32_t, WhistCursorCacheEntry> cursor_cache;
    int next_cache_access_index = 0;
    int max_entries = 0;
    bool store_data = false;
};

size_t whist_cursor_info_get_size(const WhistCursorInfo* cursor_info) {
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
    static const char* const map[] = {
        "none",         "png",        "alias",          "all-scroll",  "arrow",      "cell",
        "context-menu", "copy",       "crosshair",      "grab",        "grabbing",   "hand",
        "help",         "ibeam",      "ibeam-vertical", "move",        "no-drop",    "not-allowed",
        "progress",     "resize-col", "resize-e",       "resize-ew",   "resize-n",   "resize-ne",
        "resize-nesw",  "resize-ns",  "resize-nw",      "resize-nwse", "resize-row", "resize-s",
        "resize-se",    "resize-sw",  "resize-w",       "wait",        "zoom-in",    "zoom-out",
    };
    return map[type];
}

WhistCursorCache* whist_cursor_cache_create(int max_entries, bool store_data) {
    WhistCursorCache* cache = new WhistCursorCache();
    cache->max_entries = max_entries;
    cache->store_data = store_data;
    return cache;
}

void whist_cursor_cache_clear(WhistCursorCache* cache) {
    // Free each entry,
    for (const auto& [hash, entry] : cache->cursor_cache) {
        free(entry.cursor_info);
    }
    // Then wipe the cache
    cache->cursor_cache.clear();
}

void whist_cursor_cache_destroy(WhistCursorCache* cache) {
    // Clear the cache then delete the main struct
    whist_cursor_cache_clear(cache);
    delete cache;
}

void whist_cursor_cache_add(WhistCursorCache* cache, WhistCursorInfo* cursor_info) {
    // Ensure that the cursor info isn't already in the cache
    FATAL_ASSERT(!cache->cursor_cache.contains(cursor_info->hash));

    // Add the entry to the cache
    WhistCursorCacheEntry new_entry;
    if (cache->store_data) {
        // Copy the entire cursor info
        size_t size = whist_cursor_info_get_size(cursor_info);
        new_entry.cursor_info = (WhistCursorInfo*)safe_malloc(size);
        memcpy(new_entry.cursor_info, cursor_info, size);
    } else {
        // Copy only the metadata,
        // And mark as cached / png_size 0
        new_entry.cursor_info = (WhistCursorInfo*)safe_malloc(sizeof(WhistCursorInfo));
        memcpy(new_entry.cursor_info, cursor_info, sizeof(WhistCursorInfo));
        new_entry.cursor_info->cached = true;
        new_entry.cursor_info->png_size = 0;
    }
    new_entry.most_recent_cache_access_index = cache->next_cache_access_index++;
    cache->cursor_cache[cursor_info->hash] = new_entry;

    // If the cache got too big,
    if (cache->cursor_cache.size() > (size_t)cache->max_entries) {
        // Find the hash of the oldest cursor in the cache
        uint32_t oldest_hash = cursor_info->hash;
        int oldest_cache_access_index = new_entry.most_recent_cache_access_index;
        for (const auto& [hash, entry] : cache->cursor_cache) {
            if (entry.most_recent_cache_access_index < oldest_cache_access_index) {
                oldest_hash = hash;
                oldest_cache_access_index = entry.most_recent_cache_access_index;
            }
        }
        // Free and remove that cursor from the cache
        free(cache->cursor_cache.at(oldest_hash).cursor_info);
        cache->cursor_cache.erase(oldest_hash);
    }
}

const WhistCursorInfo* whist_cursor_cache_check(WhistCursorCache* cache, uint32_t hash) {
    if (!cache->cursor_cache.contains(hash)) {
        // Not found
        return NULL;
    }

    // Return the cursor and update its most recent access index
    cache->cursor_cache[hash].most_recent_cache_access_index = cache->next_cache_access_index++;
    return cache->cursor_cache[hash].cursor_info;
}

extern "C" {
WhistCursorManager* whist_cursor_capture_init(WhistCursorKind kind, void *args)
{
	switch (kind) {
	case WHIST_CURSOR_X11:
		return x11_cursor_new(args);
#if 0
	case WHIST_CURSOR_WIN32:
		return win32_cursor_new(args);
	case WHIST_CURSOR_WESTON:
		return weston_cursor_new(args);
#endif
	default:
		FATAL_ASSERT("unknown cursor type");
		return NULL;
	}
}
}

