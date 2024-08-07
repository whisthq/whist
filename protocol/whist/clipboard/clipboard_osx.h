#ifndef CLIPBOARD_OSX_H
#define CLIPBOARD_OSX_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file clipboard_osx.h
 * @brief This file contains the code to interface with the MacOS clipboard via
 *        Apple's Objective-C language.
============================
Usage
============================

The OSXImage and OSXFilenames structs define data for images and files in MacOS.

You can find whether the clipboard has a specific data (Image, String or File)
by calling the respective "ClipboardHas___" function. You can see whether the
clipboard updated by calling GetClipboardChangeCount you can then either
retrieve from the clipboard, or put some specific data in the clipboard, via the
respective "ClipboardGet___" or "ClipboardSet___".
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>

/*
============================
Defines
============================
*/

#define MAX_URLS 9000

/*
============================
Custom Types
============================
*/

typedef struct OSXImage {
    int size;
    unsigned char* data;
} OSXImage;

typedef struct OSXFilenames {
    char* fullPath;
    char* filename;
} OSXFilenames;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Check if the clipboard has updated since last
 *                                 time we checked it
 *
 * @returns                        The integer count of clipboard updates
 */
int get_clipboard_changecount(void);

/**
 * @brief                          Check if the clipboard has a string stored
 *
 * @returns                        True if it contains a string, else False
 */
bool check_clipboard_has_string(void);

/**
 * @brief                          Check if the clipboard is storing anything. When the clipboard
 *                                 has items in it this will always return true! However, when some
 *                                 other applications writes to the clipboard they will first claim
 *                                 ownership and change the clipboard change_count which will be
 *                                 reflected in get_clipboard_changecount(). This also clears all of
 *                                 the data on the clipboard - this is when it returns false. We
 *                                 use this to wait out the delay between an ownership change and
 * the actual writing/availability of the data on that clipboard.
 *
 *
 * @returns                        True if clipboard has resident(s), false otherwise
 */
bool check_clipboard_has_data_ready(void);

/**
 * @brief                          Check if the clipboard has an image stored
 *
 *
 * @returns                        True if it contains an image, else False
 */
bool check_clipboard_has_image(void);

/**
 * @brief                          Check if the clipboard has files stored
 *
 * @returns                        True if it contains files, else False
 */
bool check_clipboard_has_files(void);

/**
 * @brief                          Get a string from the clipboard
 *
 * @returns                        The string retrieved from the clipboard
 */
const char* clipboard_get_string(void);

/**
 * @brief                          Set the clipboard to a specific string
 *
 * @param str                      The string to set the clipboard to
 */
void clipboard_set_string(const char* str);

/**
 * @brief                          Get an image from the clipboard
 *
 * @param clipboard_image          The image struct that the image gets
 *                                 retrieved into
 */
void clipboard_get_image(OSXImage* clipboard_image);

/**
 * @brief                          Set the clipboard to a specific image
 *
 * @param img                      The data of the image to set the clipboard to
 *
 * @param len                      The length of the image data buffer
 */
void clipboard_set_image(char* img, int len);

/**
 * @brief                          Get one or many files from the clipboard
 *
 * @param filenames                List of the filenames retrieved from the
 *                                 clipboard
 */
void clipboard_get_files(OSXFilenames* filenames[]);

/**
 * @brief                          Set the clipboard to one or many files
 *
 * @param filepaths                List of file paths to set the clipboard to
 */
void clipboard_set_files(char* filepaths[]);

#endif  // CLIPBOARD_OSX_H
