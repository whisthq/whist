/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file whist_string.h
 * @brief Helper functions for string manipulations.
 */
#ifndef WHIST_CORE_WHIST_STRING_H
#define WHIST_CORE_WHIST_STRING_H

#include <stdbool.h>
#include <stddef.h>

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
 * @brief                          Trim a string to remove any dangling multi-byte Unicode
 *                                 characters that may have been left over from a previous
 *                                 truncation.
 *
 * @note                           If a multi-byte Unicode character is split by a truncation
 *                                 and not trimmed using this function, then horrible crashes
 *                                 can occur.
 *
 * @param str                      The null-terminated UTF-8 string to be trimmed.
 */
void trim_utf8_string(char* str);

/**
 * @brief                          Split a string at the first occurence of a delimiter.
 *
 * @param str                      The null-terminated string to be split. The string is
 *                                 modified so that the first occurence of the delimiter is
 *                                 replaced with a null termination.
 *
 * @param delim                    A null-terminated string containing possible delimiters.
 *
 * @returns                        A pointer to the first character of the second string.
 */
char* split_string_at(char* str, const char* delim);

/**
 * @brief                          Trim a string up to the first occurence of a newline character;
 *                                 that is, `\r` or `\n`.
 *
 * @param str                      The null-terminated string to be trimmed.
 */
void trim_newlines(char* str);

#endif  // WHIST_CORE_WHIST_STRING_H
