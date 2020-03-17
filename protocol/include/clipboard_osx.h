// header file for the MacOS clipboard functions
// 
// By: Philippe Noël
//
#ifndef CLIPBOARD_OSX_H
#define CLIPBOARD_OSX_H

#pragma once
#include "fractal.h"

int GetClipboardChangecount();
bool ClipboardHasString();
bool ClipboardHasImage();
const char* ClipboardGetString();










struct CGImage* ClipboardGetImage();





#endif CLIPBOARD_OSX_H