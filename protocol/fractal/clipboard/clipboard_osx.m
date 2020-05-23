/*
 * Objective-C MacOS clipboard setting and getting functions.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "clipboard_osx.h"
#include <AppKit/AppKit.h>

int GetClipboardChangecount() {
  NSInteger changeCount = [[NSPasteboard generalPasteboard] changeCount];
  return (int) changeCount;
}

bool ClipboardHasString() {
  return [[NSPasteboard generalPasteboard]
          canReadObjectForClasses:[NSArray arrayWithObject:
          [NSString class]] options:[NSDictionary dictionary]];
}

bool ClipboardHasImage() {
  return [[NSPasteboard generalPasteboard]
          canReadObjectForClasses:[NSArray arrayWithObject:
          [NSImage class]] options:[NSDictionary dictionary]];
}

const char *ClipboardGetString() {
  // attempt to read the strings from the clipboard
  NSString *text = [[[NSPasteboard generalPasteboard]
                    readObjectsForClasses:[NSArray arrayWithObject:
                    [NSString class]] options:[NSDictionary dictionary]] firstObject];
  if (!text) {
    printf("Can't get Mac Clipboard String data // No String data to get.\n");
    return "";  // empty string since there is no clipboard text data
  } else {
    // convert to const char* and return
    return [text UTF8String];
  }
}

void ClipboardSetString(const char *str) {
  [[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
  [[NSPasteboard generalPasteboard] setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
  return;
}

void ClipboardGetImage(OSXImage *clipboard_image) {
  // create a bitmap image from the content of the clipboard
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSBitmapImageRep *imageRep = [NSBitmapImageRep* imageRepWithPasteboard:pasteboard];
  NSDictionary *properties = [NSDictionary dictionary];

  // attempt to get the image from the clipboard
  NSData *imageData = [imageRep representationUsingType:NSBitmapImageFileTypeBMP properties:properties];
  clipboard_image->size = [imageData length];
  clipboard_image->data = [imageData bytes];
  if (clipboard_image->size == 0) {
    printf("Can't get Mac Clipboard Image data // No Image data to get.\n");
  }
  return;
}

void ClipboardSetImage(char *img, int len) {
  NSData *imageData = [[[NSData alloc] initWithBytes:img length:len] autorelease];
  NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc] initWithData:imageData] autorelease];
  NSImage *image = [[[NSImage alloc] initWithSize:[imageRep size]] autorelease];
  [image addRepresentation:imageRep];
  [[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSPasteboardTypeTIFF]
                                           owner:nil];
  [[NSPasteboard generalPasteboard] setData:[image TIFFRepresentation]
                                    forType:NSPasteboardTypeTIFF];
  return;
}

bool ClipboardHasFiles() {
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSArray *classArray = [NSArray arrayWithObject:[NSURL class]];
  NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                      forKey:NSPasteboardURLReadingFileURLsOnlyKey];
  return [pasteboard canReadObjectForClasses:classArray options:options];
}

void ClipboardGetFiles(OSXFilenames *filenames[]) {
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  NSArray *classArray = [NSArray arrayWithObject:[NSURL class]];

  // only file URLs
  NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                      forKey:NSPasteboardURLReadingFileURLsOnlyKey];

  // attempt to get the files and return
  NSArray *fileURLs = [pasteboard readObjectsForClasses:classArray options:options];
  NSUInteger i;
  for (i = 0; i < (NSUInteger)[fileURLs count]; i++) {
    strcpy(filenames[i]->fullPath, [fileURLs[i] fileSystemRepresentation]);
    strcpy(filenames[i]->filename, [[fileURLs[i] lastPathComponent] UTF8String]);
  }
  // error checking
  if (i == 0) {
    printf("Can't get Mac Clipboard Files data. / no File data to get.\n");
  }
  return;
}

void ClipboardSetFiles(char *filepaths[]) {
  // clear clipboard
  [[NSPasteboard generalPasteboard] clearContents];
  // create NSArray of NSURLs
  NSMutableArray *mutableArrURLs = [NSMutableArray arrayWithCapacity:MAX_URLS];

  // convert
  for (size_t i = 0; i < MAX_URLS; i++) {
    if (*filepaths[i] != '\0') {
      NSString *urlString = [NSString stringWithUTF8String:filepaths[i]];
      NSURL *url = [[NSURL fileURLWithPath:urlString] absoluteURL];
      if (url == nil) {
        printf("Error in converting C string relative path to NSURL.\n");
      }
      [mutableArrURLs addObject:url];
    } else {
      break;
    }
  }
  [[NSPasteboard generalPasteboard] writeObjects:mutableArrURLs];
}
