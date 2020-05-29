#ifndef CLIPBOARD_OSX_H
#define CLIPBOARD_OSX_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clipboard_osx.h
 * @brief This file contains the code to interface with the MacOS clipboard via Apple's Objective-C language.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#pragma once
#include "../core/fractal.h"

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
 * @brief                          Check if the clipboard has updated since last time we checked it
 * 
 * @returns                        The integer count of clipboard updates (e.g. 233, and after an update, 234)
 */
int GetClipboardChangecount();

/**
 * @brief                          Check if the clipboard has a string stored in it
 * 
 * @returns                        True if it contains a string, else False 
 */
bool ClipboardHasString();

/**
 * @brief                          Check if the clipboard has an image stored in it
 * 
 * @returns                        True if it contains an image, else False 
 */
bool ClipboardHasImage();

/**
 * @brief                          Check if the clipboard has files stored in it
 * 
 * @returns                        True if it contains files, else False 
 */
bool ClipboardHasFiles();

/**
 * @brief                          Get a string from the clipboard
 * 
 * @returns                        The string retrieved from the clipboard
 */
const char* ClipboardGetString();

/**
 * @brief                          Set the clipboard to a specific string
 * 
 * @param str                      The string to set the clipboard to 
 */
void ClipboardSetString(const char* str);

/**
 * @brief                          Get an image from the clipboard
 * 
 * @param clipboard_image          The image struct that the image gets retrieved into
 */
void ClipboardGetImage(OSXImage* clipboard_image);

/**
 * @brief                          Set the clipboard to a specific image
 * 
 * @param img                      The data of the image to set the clipboard to
 * @param len                      The length of the image data buffer 
 */
void ClipboardSetImage(char* img, int len);

/**
 * @brief                          Get one or many files from the clipboard 
 * 
 * @param filenames                List of the filenames retrieved from the clipboard
 */
void ClipboardGetFiles(OSXFilenames* filenames[]);

/**
 * @brief                          Set the clipboard to one or many files
 * 
 * @param filepaths                List of file paths to set the clipboard to
 */
void ClipboardSetFiles(char* filepaths[]);

#endif  // CLIPBOARD_OSX_H
