/*
 * Objective-C MacOS clipboard setting and getting functions.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "clipboard_osx.h"

#include <AppKit/AppKit.h>

int GetClipboardChangecount() {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSInteger changeCount = [pasteboard changeCount];
    return (int)changeCount;
}

bool ClipboardHasString() {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];
    return [pasteboard canReadObjectForClasses:classArray options:options];
}

bool ClipboardHasImage() {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSImage class]];
    NSDictionary *options = [NSDictionary dictionary];
    return [pasteboard canReadObjectForClasses:classArray options:options];
}

const char *ClipboardGetString() {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];

    if ([pasteboard canReadObjectForClasses:classArray options:options]) {
        NSArray *objectsToPaste = [pasteboard readObjectsForClasses:classArray
                                                            options:options];
        NSString *text = [objectsToPaste firstObject];
        if (!text) {
            return "";  // empty string since there is no clipboard text data
        } else {
            // convert to const char* and return
            return [text UTF8String];
        }
    } else {
        printf("Can't get Mac Clipboard String data.\n");
        return "";  // empty string since there is no clipboard test data
    }
}

void ClipboardSetString(const char *str) {
    [[NSPasteboard generalPasteboard]
        declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString]
               owner:nil];
    [[NSPasteboard generalPasteboard]
        setString:[NSString stringWithUTF8String:str]
          forType:NSPasteboardTypeString];
    return;
}

void ClipboardGetImage(OSXImage *clipboard_image) {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSBitmapImageRep *rep = (NSBitmapImageRep *)[NSBitmapImageRep
        imageRepWithPasteboard:pasteboard];
    NSDictionary *properties = [NSDictionary dictionary];

    if (rep) {
        // get the data
        NSData *data = [rep representationUsingType:NSBitmapImageFileTypeBMP
                                         properties:properties];
        // set fields and return
        clipboard_image->size = [data length];
        clipboard_image->data = (unsigned char *)[data bytes];
        return;
    } else {
        // no image in clipboard
        return;
    }
}

void ClipboardSetImage(char *img, int len) {
    NSData *imageData = [[[NSData alloc] initWithBytes:img
                                                length:len] autorelease];
    NSBitmapImageRep *imageRep =
        [[[NSBitmapImageRep alloc] initWithData:imageData] autorelease];
    NSImage *image =
        [[[NSImage alloc] initWithSize:[imageRep size]] autorelease];
    [image addRepresentation:imageRep];
    [[NSPasteboard generalPasteboard]
        declareTypes:[NSArray arrayWithObject:NSPasteboardTypeTIFF]
               owner:nil];
    [[NSPasteboard generalPasteboard] setData:[image TIFFRepresentation]
                                      forType:NSPasteboardTypeTIFF];
    return;
}

bool ClipboardHasFiles() {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSURL class]];
    NSDictionary *options = [NSDictionary
        dictionaryWithObject:[NSNumber numberWithBool:YES]
                      forKey:NSPasteboardURLReadingFileURLsOnlyKey];
    return [pasteboard canReadObjectForClasses:classArray options:options];
}

void ClipboardGetFiles(OSXFilenames *filenames[]) {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSURL class]];

    // only file URLs
    NSDictionary *options = [NSDictionary
        dictionaryWithObject:[NSNumber numberWithBool:YES]
                      forKey:NSPasteboardURLReadingFileURLsOnlyKey];

    if ([pasteboard canReadObjectForClasses:classArray options:options]) {
        NSArray *fileURLs = [pasteboard readObjectsForClasses:classArray
                                                      options:options];
        for (NSUInteger i = 0; i < [fileURLs count]; i++) {
            strcpy(filenames[i]->fullPath,
                   [fileURLs[i] fileSystemRepresentation]);
            strcpy(filenames[i]->filename,
                   [[fileURLs[i] lastPathComponent] UTF8String]);
        }
    } else {
        printf("Can't get Mac Clipboard Files data.\n");
    }
}

void ClipboardSetFiles(char *filepaths[]) {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

    // clear pasteboard
    [pasteboard clearContents];

    // create NSArray of NSURLs
    NSMutableArray *mutableArrURLs =
        [NSMutableArray arrayWithCapacity:MAX_URLS];

    // convert
    for (size_t i = 0; i < MAX_URLS; i++) {
        if (*filepaths[i] != '\0') {
            NSString *urlString = [NSString stringWithUTF8String:filepaths[i]];
            NSURL *url = [[NSURL fileURLWithPath:urlString] absoluteURL];
            if (url == nil) {
                printf(
                    "Error in converting C string relative path to NSURL.\n");
            }
            [mutableArrURLs addObject:url];
        } else {
            break;
        }
    }
    [pasteboard writeObjects:mutableArrURLs];
}
