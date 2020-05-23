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
    return [[[[NSPasteboard generalPasteboard]
            readObjectsForClasses:[NSArray arrayWithObject:
            [NSString class]] options:[NSDictionary dictionary]] firstObject] UTF8String];
}

void ClipboardSetString(const char *str) {
  [[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
  [[NSPasteboard generalPasteboard] setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
  return;
}

void ClipboardGetImage(OSXImage *clipboard_image) {
  clipboard_image->size = [[[NSBitmapImageRep* imageRepWithPasteboard:[NSPasteboard generalPasteboard]]
                            representationUsingType:NSBitmapImageFileTypeBMP properties:[NSDictionary dictionary]] length];
  clipboard_image->data = [[[NSBitmapImageRep* imageRepWithPasteboard:[NSPasteboard generalPasteboard]]
                            representationUsingType:NSBitmapImageFileTypeBMP properties:[NSDictionary dictionary]] bytes];
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
  NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                      forKey:NSPasteboardURLReadingFileURLsOnlyKey];
  return [[NSPasteboard generalPasteboard] canReadObjectForClasses:[NSArray arrayWithObject:[NSURL class]] options:options];
}

void ClipboardGetFiles(OSXFilenames *filenames[]) {
  // only file URLs
  NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                      forKey:NSPasteboardURLReadingFileURLsOnlyKey];

  // attempt to get the files and return
  NSArray *fileURLs = [[NSPasteboard generalPasteboard] readObjectsForClasses:[NSArray arrayWithObject:[NSURL class]] options:options];
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
