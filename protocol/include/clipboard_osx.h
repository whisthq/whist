#ifndef CLIPBOARD_OSX_H
#define CLIPBOARD_OSX_H

#define MAX_URLS 9000

#pragma once

typedef struct OSXImage
{
    int size;
    unsigned char* data;
} OSXImage;

typedef struct OSXFilenames
{
    char* fullPath;
    char* filename;
} OSXFilenames;

int GetClipboardChangecount();
bool ClipboardHasString();
bool ClipboardHasImage();
bool ClipboardHasFiles();
const char* ClipboardGetString();
void ClipboardSetString( const char* str );
void ClipboardGetImage( OSXImage* clipboard_image );
void ClipboardSetImage( char* img, int len );
void ClipboardGetFiles( OSXFilenames* filenames[] );
void ClipboardSetFiles( char* filepaths[] );

#endif // CLIPBOARD_OSX_H
