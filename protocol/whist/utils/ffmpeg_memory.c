/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file ffmpeg_memory.c
 * @brief Memory allocation overrides for FFmpeg on macOS.
 */

#include <stdlib.h>
#include <unistd.h>

// These functions have no visible prototype - they are only called by
// the memory allocation code for FFmpeg in libavutil/mem.c.  Their
// symbols must be global because they are resolved at runtime by the
// dynamic linker.
#pragma clang diagnostic ignored "-Wmissing-prototypes"

void *whist_ffmpeg_malloc(size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t align;
    if (size >= page_size) {
        // Align to page size so that buffers can be used with SDL.
        align = page_size;
    } else {
        // Small buffers only need the largest-register alignment as
        // normally required by FFmpeg.
        align = 64;
    }
    void *ptr;
    if (posix_memalign(&ptr, align, size)) ptr = NULL;
    return ptr;
}

void *whist_ffmpeg_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void whist_ffmpeg_free(void *ptr) { free(ptr); }
