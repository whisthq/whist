/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file string_buffer.h
 * @brief API for string buffers.
 */
#ifndef WHIST_UTILS_STRING_BUFFER_H
#define WHIST_UTILS_STRING_BUFFER_H

#include <stddef.h>
#include <stdarg.h>

/**
 * String buffer object.
 *
 * @private
 * This should be treated as opaque.
 */
typedef struct StringBuffer {
    /** The underlying buffer. */
    char *data;
    /** The current write location in the buffer. */
    size_t current;
    /** Size of the buffer. */
    size_t size;
} StringBuffer;

#define STRING_BUFFER_LOCAL(name, size)      \
    char name##_string_buffer_storage[size]; \
    StringBuffer name = {name##_string_buffer_storage, 0, size}

/**
 * Initialise a new string buffer.
 *
 * @param sb    String buffer to initialise.
 * @param buf   Buffer to use.
 * @param size  Size of the buffer.
 */
void string_buffer_init(StringBuffer *sb, char *buf, size_t size);

/**
 * Write a string to a string buffer.
 *
 * @param sb   String buffer to write to.
 * @param str  String to write.
 */
void string_buffer_puts(StringBuffer *sb, const char *str);

/**
 * Write to a string buffer in vsnprintf() style.
 *
 * @param sb      String buffer to write to.
 * @param format  Format string.
 * @param args    Arguments for the format string.
 */
void string_buffer_vprintf(StringBuffer *sb, const char *format, va_list args);

/**
 * Write to a string buffer in snprintf() style.
 *
 * @param sb      String buffer to write to.
 * @param format  Format string.
 */
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void string_buffer_printf(StringBuffer *sb, const char *format, ...);

/**
 * Return a pointer to the string inside a string buffer.
 *
 * @param sb  String buffer to query.
 * @return  Pointer to the string.
 */
const char *string_buffer_string(StringBuffer *sb);

/**
 * Find the number of bytes written to a string buffer.
 *
 * @param sb  String buffer to query.
 * @return  Bytes written (not including terminator).
 */
size_t string_buffer_written(StringBuffer *sb);

/**
 * Find the number of bytes remaining in a string buffer.
 *
 * @param sb  String buffer to query.
 * @return  Byte remaining (not including terminator).
 */
size_t string_buffer_remaining(StringBuffer *sb);

#endif /* WHIST_UTILS_STRING_BUFFER_H */
