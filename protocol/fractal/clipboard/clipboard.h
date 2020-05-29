#ifndef CLIPBOARD_H
#define CLIPBOARD_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clipboard.h
 * @brief This file contains the general clipboard functions for a shared client-server clipboard. 
============================
Usage
============================

GET_CLIPBOARD and SET_CLIPBOARD will return strings representing directories important for getting and setting file
clipboards. When GetClipboard() is called and it returns a CLIPBOARD_FILES type, then GET_CLIPBOARD will be filled 
with symlinks to the clipboard files. When SetClipboard(cb) is called and is given a clipboard with a CLIPBOARD_FILES type,
then the clipboard will be set to whatever files are in the SET_CLIPBOARD directory.

LGET_CLIPBOARD and LSET_CLIPBOARD are the wide-character versions of these strings, for use on windows OS's
*/

/*
============================
Includes
============================
*/

#include <stdbool.h>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4200)
#include <windows.h>
#endif



/*
============================
Defines
============================
*/

#ifdef _WIN32
WCHAR* lget_clipboard_directory();
WCHAR* lset_clipboard_directory();
char* get_clipboard_directory();
char* set_clipboard_directory();
#define LGET_CLIPBOARD (lget_clipboard_directory())
#define GET_CLIPBOARD (get_clipboard_directory())
#define LSET_CLIPBOARD (lset_clipboard_directory())
#define SET_CLIPBOARD (set_clipboard_directory())
#else
#define GET_CLIPBOARD "./get_clipboard"
#define SET_CLIPBOARD "./set_clipboard"
#endif

/*
============================
Custom types
============================
*/

/**
 * @brief                          The type of data that a clipboard might be
 */
typedef enum ClipboardType {
  CLIPBOARD_NONE,
  CLIPBOARD_TEXT,
  CLIPBOARD_IMAGE,
  CLIPBOARD_FILES
} ClipboardType;

/**
 * @brief                          A packet of data referring to and containing the information of a clipboard
 */
typedef struct ClipboardData {
  int size;            // Number of bytes for the clipboard data
  ClipboardType type;  // The type of data for the clipboard
  char data[];         // The data that stores the clipboard information
} ClipboardData;

/**
 * @brief                          Data packet description
 */
typedef struct ClipboardFiles {
  int size;
  char* files[];
} ClipboardFiles;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Initialize the clipboard to put/get data to/from
 */
void initClipboard();

/**
 * @brief                          Get the current clipboard data
 *
 * @returns                        Pointer to the current clipboard data as a ClipboardData struct
 */
ClipboardData* GetClipboard();

/**
 * @brief                         Set the clipboard to the given clipboard data
 *
 * @param cb                      Pointer to a clipboard data struct to set the clipboard to
 */
void SetClipboard(ClipboardData* cb);

/**
 * @brief                          Get the current clipboard
 *
 * @returns                        Pointer to the current clipboard data
 */
bool hasClipboardUpdated();

#endif  // CLIPBOARD_H
