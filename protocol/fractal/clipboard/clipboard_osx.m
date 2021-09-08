/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clipboard_osx.m
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

#include "clipboard_osx.h"
#include <AppKit/AppKit.h>

/*
============================
Public Function Implementations
============================
*/

int get_clipboard_changecount() {
    /*
        Check if the clipboard has updated since last time we checked it

        Returns:
            (int): The integer count of clipboard updates (e.g. 233, and after an
                update, 234)
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSInteger change_count = [pasteboard changeCount];
    return (int)change_count;
}

bool check_clipboard_has_string() {
    /*
        Check if the clipboard has a string stored

        Returns:
            (bool): true if it contains a string, else false
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *class_array = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];
    return [pasteboard canReadObjectForClasses:class_array options:options];
}

bool check_clipboard_has_image() {
    /*
        Check if the clipboard has an image stored

        Returns:
            (bool): true if it contains a string, else false
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *class_array = [NSArray arrayWithObject:[NSImage class]];
    NSDictionary *options = [NSDictionary dictionary];
    return [pasteboard canReadObjectForClasses:class_array options:options];
}

const char *clipboard_get_string() {
    /*
        Get a string from the clipboard

        Returns:
            (const char*): The string retrieved from the clipboard
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *class_array = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];

    if ([pasteboard canReadObjectForClasses:class_array options:options]) {
        NSArray *objects_to_paste = [pasteboard readObjectsForClasses:class_array options:options];
        NSString *text = [objects_to_paste firstObject];
        if (!text) {
            return "";  // empty string since there is no clipboard text data
        } else {
            // convert to const char* and return
            return [text UTF8String];
        }
    } else {
        LOG_ERROR("Can't get Mac Clipboard String data.");
        return "";  // empty string since there is no clipboard test data
    }
}

void clipboard_set_string(const char *str) {
    /*
        Set the clipboard to a specific string

        Arguments:
            str (const char*): The string to set the clipboard to
    */

    // clear clipboard and then set string data
    [[NSPasteboard generalPasteboard] clearContents];
    [[NSPasteboard generalPasteboard] setString:[NSString stringWithUTF8String:str]
                                        forType:NSPasteboardTypeString];
    return;
}

void clipboard_get_image(OSXImage *clipboard_image) {
    /*
        Get an image from the clipboard

        Arguments:
            clipboard_image (OSXImage*): The image struct that the image gets
                retrieved into
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSBitmapImageRep *rep =
        (NSBitmapImageRep *)[NSBitmapImageRep imageRepWithPasteboard:pasteboard];
    NSDictionary *properties = [NSDictionary dictionary];

    if (rep) {
        // get the data
        NSData *data = [rep representationUsingType:NSBitmapImageFileTypePNG properties:properties];
        // set fields and return
        clipboard_image->size = [data length];
        clipboard_image->data = (unsigned char *)[data bytes];
        return;
    } else {
        // no image in clipboard
        return;
    }
}

void clipboard_set_image(char *img, int len) {
    /*
        Set the clipboard to a specific image

        Arguments:
            img (char*): The data of the image to set the clipboard to
            len (int): The length of the image data buffer
    */

    NSData *image_data = [[[NSData alloc] initWithBytes:img length:len] autorelease];
    NSBitmapImageRep *image_rep = [[[NSBitmapImageRep alloc] initWithData:image_data] autorelease];
    NSImage *image = [[[NSImage alloc] initWithSize:[image_rep size]] autorelease];
    [image addRepresentation:image_rep];

    // clear clipboard and then set image data
    [[NSPasteboard generalPasteboard] clearContents];
    [[NSPasteboard generalPasteboard] setData:[image TIFFRepresentation]
                                      forType:NSPasteboardTypeTIFF];
    return;
}

bool check_clipboard_has_files() {
    /*
        Check if the clipboard has files stored

        Returns:
            (bool): true if it contains files, else false
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *class_array = [NSArray arrayWithObject:[NSURL class]];
    NSDictionary *options =
        [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                    forKey:NSPasteboardURLReadingFileURLsOnlyKey];
    return [pasteboard canReadObjectForClasses:class_array options:options];
}

void clipboard_get_files(OSXFilenames *filenames[]) {
    /*
        Get one or many files from the clipboard

        Arguments:
            filenames (OSXFilenames*[]): List of the filenames retrieved from the
                clipboard
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *class_array = [NSArray arrayWithObject:[NSURL class]];

    // only file URLs
    NSDictionary *options =
        [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                    forKey:NSPasteboardURLReadingFileURLsOnlyKey];

    if ([pasteboard canReadObjectForClasses:class_array options:options]) {
        NSArray *file_urls = [pasteboard readObjectsForClasses:class_array options:options];
        for (NSUInteger i = 0; i < [file_urls count]; i++) {
            // TODO(anton) if it is possible that a file path can be longer than PATH_MAXLEN+1, then we
            // may want to check the return value of safe_strncpy to see if the file path was
            // truncated
            safe_strncpy(filenames[i]->fullPath, [file_urls[i] fileSystemRepresentation], PATH_MAXLEN+1);
            safe_strncpy(filenames[i]->filename, [[file_urls[i] lastPathComponent] UTF8String],
                         PATH_MAXLEN+1);
        }
    } else {
        LOG_ERROR("Can't get Mac Clipboard Files data.");
    }
}

void clipboard_set_files(char *filepaths[]) {
    /*
        Set the clipboard to one or many files

        Arguments:
            filepaths (char*[]): List of file paths to set the clipboard to
    */

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

    // clear pasteboard
    [pasteboard clearContents];

    // create NSArray of NSURLs
    NSMutableArray *mutable_arr_urls = [NSMutableArray arrayWithCapacity:MAX_URLS];

    // convert
    for (size_t i = 0; i < MAX_URLS; i++) {
        if (*filepaths[i] != '\0') {
            NSString *url_string = [NSString stringWithUTF8String:filepaths[i]];
            NSURL *url = [[NSURL fileURLWithPath:url_string] absoluteURL];
            if (url == nil) {
                LOG_ERROR("Error in converting C string relative path to NSURL.");
            }
            [mutable_arr_urls addObject:url];
        } else {
            break;
        }
    }
    [pasteboard writeObjects:mutable_arr_urls];
}
