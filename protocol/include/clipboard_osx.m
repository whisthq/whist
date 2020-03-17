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

const char *ClipboardGetString() {
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

OSXImage *ClipboardGetImage() {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSBitmapImageRep *rep = (NSBitmapImageRep*)[NSBitmapImageRep imageRepWithPasteboard:pasteboard];
	if( rep ) {
        // get the data
        NSData *data = [rep representationUsingType:NSBMPFileType properties:nil];

        // create OSX image struct
        OSXImage *clipboard_image = (OSXImage *) malloc(sizeof(OSXImage));
        memset(clipboard_image, 0, sizeof(OSXImage));

        // set fields and return
        clipboard_image->size = [data length];
        clipboard_image->data = [data bytes];
        return clipboard_image;
    }
	else {
        // no image in clipboard
		return nil;
    }
}







void ClipboardSetString(const char *str) {
	[[NSPasteboard generalPasteboard] declareTypes: [NSArray arrayWithObject: NSPasteboardTypeString] owner:nil];
	[[NSPasteboard generalPasteboard] setString:[NSString stringWithUTF8String:str] forType: NSPasteboardTypeString];
    return;
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










/*

void ClipboardSetImage( ImageSourceRef imageSource, ImageTarget::Options options )
{


	cocoa::ImageTargetCgImageRef target = cocoa::ImageTargetCgImage::createRef( imageSource, options );
	imageSource->load( target );
	target->finalize();
	
	NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc] initWithCGImage:target->getCgImage()] autorelease];
	NSImage *image = [[NSImage alloc] initWithSize:[imageRep size]];
	[image addRepresentation: imageRep];
	[[NSPasteboard generalPasteboard] declareTypes: [NSArray arrayWithObject: NSTIFFPboardType] owner:nil];
	[[NSPasteboard generalPasteboard] setData:[image TIFFRepresentation] forType:NSTIFFPboardType];	
	[image release];
    return;
}
*/


// TODO add set