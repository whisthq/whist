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

#include <fractal/core/fractal.h>
#include "window_name.h"

#if defined(_WIN32)

// TODO: implement functionality for windows servers
void init_window_name_getter() { LOG_ERROR("UNIMPLEMENTED: init_window_name_getter on Win32"); }
int get_focused_window_name(char* name_return) {
    LOG_ERROR("UNIMPLEMENTED: get_focused_window_name on Win32");
    return -1;
}
void destroy_window_name_getter() {
    LOG_ERROR("UNIMPLEMENTED: destroy_window_name_getter on Win32");
}

#elif __linux__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

Display* display;

void init_window_name_getter() {
    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }
}

#define ONE_BYTE_MAX_UNICODE_CODEPOINT 0x7F
#define TWO_BYTES_MAX_UNICODE_CODEPOINT 0x07FF
#define THREE_BYTES_MAX_UNICODE_CODEPOINT 0xFFFF
#define FOUR_BYTES_MAX_UNICODE_CODEPOINT 0x10FFFF

// Codepoint to UTF8 char encoding algorithm & bit masks acknowledged to
// https://gist.github.com/MightyPork/52eda3e5677b4b03524e40c9f0ab1da5
size_t convert_string_to_utf8_format(char* string_output, char* string_input,
                                     size_t max_output_length) {
    /*
        Converts a string of single-byte chars into one encoded according to UTF-8

        Arguments:
            string_output (char*): The output string, encoded in UTF-8 format. Each character may
                                   take up more than 1 byte.
            string_input (char*): the input string, unencoded. Each char in string_input contains
                                  the codepoint of the desired UTF-8 character, but the codepoints
                                  may be outside the ASCII range.
        Return:
            ret (int): The number of characters in the original string which didn't fit
                       in the string once encoded in UTF-8 format (UTF-8 characters are
                        encoded using up to 4 bytes)
    */

    size_t len = strlen(string_input);
    size_t index_in = 0, index_out = 0;

    memset(string_output, 0, max_output_length);

    for (; index_in < len; index_in++) {
        uint32_t codepoint = (uint32_t)(unsigned char)string_input[index_in];

        if (codepoint <= ONE_BYTE_MAX_UNICODE_CODEPOINT) {
            if (index_out + 1 > max_output_length) {
                break;
            }
            string_output[index_out] = (char)codepoint;
            index_out += 1;
        } else if (codepoint <= TWO_BYTES_MAX_UNICODE_CODEPOINT) {
            if (index_out + 2 > max_output_length) {
                break;
            }
            string_output[index_out] = (char)(((codepoint >> 6) & 0x1F) | 0xC0);
            string_output[index_out + 1] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
            index_out += 2;
        } else if (codepoint <= THREE_BYTES_MAX_UNICODE_CODEPOINT) {
            if (index_out + 3 > max_output_length) {
                break;
            }
            string_output[index_out] = (char)(((codepoint >> 12) & 0x0F) | 0xE0);
            string_output[index_out + 1] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
            string_output[index_out + 2] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
            index_out += 3;
        } else if (codepoint <= FOUR_BYTES_MAX_UNICODE_CODEPOINT) {
            if (index_out + 4 > max_output_length) {
                break;
            }
            string_output[index_out] = (char)(((codepoint >> 18) & 0x07) | 0xF0);
            string_output[index_out + 1] = (char)(((codepoint >> 12) & 0x3F) | 0x80);
            string_output[index_out + 2] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
            string_output[index_out + 3] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
            index_out += 4;
        } else {
            if (index_out + 3 > max_output_length) {
                break;
            }

            // Encoding error, use replacement character instead
            string_output[index_out] = (char)0xEF;
            string_output[index_out + 1] = (char)0xBF;
            string_output[index_out + 2] = (char)0xBD;
            index_out += 3;
        }
    }

    return len - index_in;
}

int get_focused_window_name(char* name_return) {
    /*
     * Get the name of the focused window.
     *
     * Arguments:
     *     name_return (char*): Pointer to location to write name. Must have at least
     *                          WINDOW_NAME_MAXLEN + 1 bytes available.
     *
     *  Return:
     *      ret (int): 0 on success, other integer on failure
     */

    Window w;
    int revert;
    XGetInputFocus(display, &w, &revert);

    if (!w) {
        // No window is active.
        return 1;
    }

    // https://gist.github.com/kui/2622504
    XTextProperty prop;
    Status s;

    s = XGetWMName(display, w, &prop);
    if (s) {
        int count = 0, result;
        char** list = NULL;
        result = XmbTextPropertyToTextList(display, &prop, &list, &count);
        if (!count) {
            // no window title found
            return 1;
        }
        if (result == Success) {
            size_t res = convert_string_to_utf8_format(name_return, list[0], WINDOW_NAME_MAXLEN + 1);

            if (res > 0) {
                LOG_ERROR(
                    "The last %i characters from the focused window title got truncated upon UTF-8 "
                    "encoding",
                    res);
            }

            XFreeStringList(list);
            return 0;
        } else {
            LOG_ERROR("XmbTextPropertyToTextList failed to convert window name to string");
        }
    } else {
        LOG_ERROR("XGetWMName failed to get the name of the focused window");
    }
    return 1;
}

void destroy_window_name_getter() {
    if (display != NULL) {
        XCloseDisplay(display);
        display = NULL;
    }
}

#endif  // __linux__
