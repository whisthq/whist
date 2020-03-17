// clipboard for MacOS in Objective-C
// 
// By: Philippe Noël
//
#include <AppKit/AppKit.h>
#include "clipboard_osx.h"

int GetClipboardChangecount() {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSInteger changeCount = [pasteboard changeCount];
    return (int) changeCount;
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

const char* ClipboardGetString() {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];
 
	if ([pasteboard canReadObjectForClasses:classArray options:options]) {
		NSArray *objectsToPaste = [pasteboard readObjectsForClasses:classArray options:options];
        NSString *text = [objectsToPaste firstObject];
		if(!text) {
			return ""; // empty string since there is no clipboard text data
        }
		else {
            // convert to const char* and return
			return [text UTF8String];
        }
	}
	else {
        mprintf("Can't get Mac Clipboard String data.\n");
		return ""; // empty string since there is no clipboard test data
	}
}














struct CGImage* ClipboardGetImage()
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSBitmapImageRep *rep = (NSBitmapImageRep*)[NSBitmapImageRep imageRepWithPasteboard:pasteboard];
	if( rep ) {
		return [rep CGImage]; // bitmap format
    }
	else {


		return "COULDN'T GET IMAGE";
    }
}



/*
ImageSourceRef Clipboard::getImage()
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSBitmapImageRep *rep = (NSBitmapImageRep*)[NSBitmapImageRep imageRepWithPasteboard:pasteboard];
	if( rep )
		return cocoa::ImageSourceCgImage::createRef( [rep CGImage] );
	else
		return ImageSourceRef();
}


*/

// TODO add set