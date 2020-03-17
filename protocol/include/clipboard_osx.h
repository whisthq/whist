#ifndef CLIPBOARD_OSX_H
#define CLIPBOARD_OSX_H

#pragma once
#include "fractal.h"

typedef struct OSXImage
{
	int size;
    unsigned char *data;
} OSXImage;

int GetClipboardChangecount();
bool ClipboardHasString();
bool ClipboardHasImage();
const char *ClipboardGetString();
OSXImage *ClipboardGetImage();








void ClipboardSetString(const char *str);


void ClipboardSetImage();





#endif // CLIPBOARD_OSX_H