#ifndef STRING_UTILS_H
#define STRING_UTILS_H
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

/*
============================
Includes
============================
*/

#include "../core/fractal.h"

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Safely copy a string from source to destination.
 *
 * @details                        Copies at most `num` bytes from `source` to `destination`. Bytes
 *                                 after the first null character of `source` are not copied.
 *                                 If no null character is encountered within the first `num` bytes
 *                                 of `source`, `destination[num - 1]` will be manually set to zero,
 *                                 so `destination` is guaranteed to be null terminated, unless
 *                                 `num` is zero, in which case the `destination` is left unchanged.
 *
 * @param destination              Address to copy to. Should have at least `num` bytes available.
 *
 * @param source                   Address to copy from.
 *
 * @param num                      Number of bytes to copy.
 *
 * @returns                        True if all bytes of source were copied (i.e. strlen(source) <=
 *                                 num - 1)
 */
bool safe_strncpy(char* destination, const char* source, size_t num);

/**
 * @brief                          Safely retrieve the next delimited token in a string.
 *
 * @details                        strtok is unsafe and the safe version in Unix systems is strtok_r
 *       						   while the safe version in Windows is strtok_s. This function calls
 *							       the respective thread-safe strtok version for each OS.
 *
 * @param str                      The string being parsed for tokens.
 *
 * @param delim                    The string delimiter.
 *
 * @param saveptr				   The save pointer, which should remain unchanged between
 *								   calls to this function.
 *
 * @returns                        True if all bytes of source were copied (i.e. strlen(source) <=
 *                                 num - 1)
 */
char* safe_strtok(char* str, char* delim, char** saveptr);

#endif  // STRING_UTILS_H
