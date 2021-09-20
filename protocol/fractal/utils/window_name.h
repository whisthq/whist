#ifndef WINDOW_NAME_H
#define WINDOW_NAME_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file window_name.h
 * @brief This file contains all the code for getting the name of a window.
============================
Usage
============================

init_window_name_getter();

char name[WINDOW_NAME_MAXLEN + 1];
get_focused_window_name(name);

destroy_window_name_getter();
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>

/*
============================
Defines
============================
*/

// MAXLENs are the max length of the string they represent, _without_ the null character.
// Therefore, whenever arrays are created or length of the string is compared, we should be
// comparing to *MAXLEN + 1
#define WINDOW_NAME_MAXLEN 127
#define ONE_BYTE_MAX_UNICODE_CODEPOINT 0x7F
#define TWO_BYTES_MAX_UNICODE_CODEPOINT 0x07FF
#define THREE_BYTES_MAX_UNICODE_CODEPOINT 0xFFFF
#define FOUR_BYTES_MAX_UNICODE_CODEPOINT 0x10FFFF

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize variables required to get window names.
 *
 */
void init_window_name_getter();

/**
 * @brief                          Convert a string into Unicode UTF-8 format
 *
 * @param string_input             The input string, unencoded. Each char in string_input contains
 *                                 the codepoint of the desired UTF-8 character, but the codepoints
 *                                 may be outside the ASCII range.
 *
 *
 * @returns                        The number of characters in the original string which didn't fit
 *                                 in the string once encoded in UTF-8 format (UTF-8 characters are
 *                                 encoded using up to 4 bytes)
 */
int convert_string_to_UTF8_format(char* string_output, char* string_input);

/**
 * @brief                          Get the name of the focused window.
 *
 * @param name_return              Address to write name. Should have at least WINDOW_NAME_MAXLEN +
 *                                 1 bytes available.
 *
 * @returns                        0 on success, any other int on failure.
 */
int get_focused_window_name(char* name_return);

/**
 * @brief                          Destroy variables that were initialized.
 *
 */
void destroy_window_name_getter();

#endif  // WINDOW_NAME_H
