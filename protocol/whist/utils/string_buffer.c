/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file string_buffer.h
 * @brief Implementation of string buffers.
 */

#include "whist/logging/logging.h"
#include "string_buffer.h"

void string_buffer_init(StringBuffer *sb, char *buf, size_t size) {
    FATAL_ASSERT(size >= 1);
    memset(sb, 0, sizeof(*sb));
    sb->data = buf;
    sb->current = 0;
    sb->size = size;
    sb->data[0] = 0;
}

void string_buffer_puts(StringBuffer *sb, const char *str) {
    while (*str && sb->current + 1 < sb->size) {
        sb->data[sb->current++] = *str++;
    }
    sb->data[sb->current] = 0;
}

void string_buffer_vprintf(StringBuffer *sb, const char *format, va_list args) {
    size_t remaining = string_buffer_remaining(sb);
    if (remaining == 0) {
        // No space in buffer.
        return;
    }
    int ret = vsnprintf(sb->data + sb->current, remaining + 1, format, args);
    if (ret < 0) {
        // Error, write nothing to buffer.
    } else if ((size_t)ret > remaining) {
        sb->current = sb->size - 1;
    } else {
        sb->current += (size_t)ret;
    }
}

void string_buffer_printf(StringBuffer *sb, const char *format, ...) {
    va_list args;
    va_start(args, format);
    string_buffer_vprintf(sb, format, args);
    va_end(args);
}

const char *string_buffer_string(StringBuffer *sb) { return sb->data; }

size_t string_buffer_written(StringBuffer *sb) { return sb->current; }

size_t string_buffer_remaining(StringBuffer *sb) { return sb->size - 1 - sb->current; }
