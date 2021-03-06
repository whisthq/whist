/**
 * Copyright Fractal Computers, Inc. 2020
 * @file string_utils.h
 * @brief This file contains code for working with strings.
============================
Usage
============================

char buf[10];
safe_strncpy(buf, some_string, 10);
*/

#include "string_utils.h"

bool safe_strncpy(char* destination, const char* source, size_t num) {
    /*
     * Safely copy a string from source to destination.
     *
     * Copies at most `num` bytes * after the first null character of `source` are not copied.
     * If no null character is encountered within the first `num` bytes
     * of `source`, `destination[num - 1]` will be manually set to zero,
     * so `destination` is guaranteed to be null terminated, unless
     * `num` is zero, in which case the `destination` is left unchanged.
     *
     * Arguments:
     *     destination: Address to copy to. Should have at least `num` bytes available.
     *     source: Address to copy from.
     *     num: Number of bytes to copy.
     *
     * Return:
     *     True if all bytes of source were copied (i.e. strlen(source) <= num - 1)
     */
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
