#ifndef CLIPBOARD_OSX_H
#define CLIPBOARD_OSX_H

#pragma once
#include "fractal.h"

int GetClipboardChangecount();
bool ClipboardHasString();
bool ClipboardHasImage();
const char* ClipboardGetString();
void ClipboardSetString(const char *str);










const unsigned char* ClipboardGetImage();





void ClipboardSetImage();





#endif // CLIPBOARD_OSX_H