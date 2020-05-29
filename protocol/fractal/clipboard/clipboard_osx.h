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






int GetClipboardChangecount();

bool ClipboardHasString();

bool ClipboardHasImage();

bool ClipboardHasFiles();

const char* ClipboardGetString();

void ClipboardSetString(const char* str);

void ClipboardGetImage(OSXImage* clipboard_image);

void ClipboardSetImage(char* img, int len);

void ClipboardGetFiles(OSXFilenames* filenames[]);

void ClipboardSetFiles(char* filepaths[]);







#endif  // CLIPBOARD_OSX_H
