/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file whist_string.c
 * @brief Helper functions for string manipulations.
 */

#include <string.h>
#include "whist_string.h"
#include "platform.h"

bool safe_strncpy(char* destination, const char* source, size_t num) {
    if (num > 0) {
        size_t i;
        for (i = 0; i < num - 1 && source[i] != '\0'; i++) {
            destination[i] = source[i];
        }
        destination[i] = '\0';
        return source[i] == '\0';
    }
    return false;
}

void trim_utf8_string(char* str) {
    if (OS_IS(OS_WIN32)) {
        // On Windows, UTF-8 seems to behave weirdly, causing major issues in the
        // below codepath. Therefore, I've disabled this function on Windows.
        // TODO: Fix this on Windows and re-enable the unit test.
        return;
    }

    // UTF-8 can use up to 4 bytes per character, so
    // in the malformed case, we would at most need to
    // trim 3 trailing bytes.
    size_t len = strlen(str);

    // Does a 4-byte sequence start at b[len - 3]?
    if (len >= 3 && (str[len - 3] & 0xf0) == 0xf0) {
        // Yes, so we need to trim 3 bytes.
        str[len - 3] = '\0';
        return;
    }

    // Does a 3 or more byte sequence start at str[len - 2]?
    if (len >= 2 && (str[len - 2] & 0xe0) == 0xe0) {
        // Yes, so we need to trim 2 bytes.
        str[len - 2] = '\0';
        return;
    }

    // Does a 2 or more byte sequence start at str[len - 1]?
    if (len >= 1 && (str[len - 1] & 0xc0) == 0xc0) {
        // Yes, so we need to trim 1 byte.
        str[len - 1] = '\0';
        return;
    }
}

char* split_string_at(char* str, const char* delim) {
    size_t pos = strcspn(str, delim);
    if (pos < strlen(str)) {
        str[pos] = '\0';
        return &str[pos + 1];
    }
    return NULL;
}

void trim_newlines(char* str) { split_string_at(str, "\r\n"); }

size_t copy_and_escape(char* dst, size_t dst_size, const char* src) {
    size_t d, s;

    // Table of escape codes.
    static const unsigned char escape_list[256] = {
        ['\b'] = 'b',
        ['\f'] = 'f',
        ['\r'] = 'r',
        ['\t'] = 't',
    };

    for (d = s = 0; src[s]; s++) {
        unsigned char esc = escape_list[src[s] & 0xff];
        if (esc) {
            if (d >= dst_size - 2) break;
            dst[d++] = '\\';
            dst[d++] = esc;
        } else {
            if (d >= dst_size - 1) break;
            dst[d++] = src[s];
        }
    }

    dst[d] = '\0';
    return d;
}
