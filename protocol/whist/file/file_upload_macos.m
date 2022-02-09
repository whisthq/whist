#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include "file_upload.h"

const char *whist_file_upload_get_picked_file(void) {
    [NSApp activateIgnoringOtherApps:YES];

    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setLevel:NSFloatingWindowLevel];
    [openPanel setAllowsMultipleSelection:NO];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanCreateDirectories:NO];
    [openPanel setMessage:@"Upload File to Whist"];
    [openPanel setPrompt:@"Upload"];

    NSString* fileName = nil;
    if ([openPanel runModal] == NSModalResponseOK) {
        for( NSURL* URL in [openPanel URLs])
        {
            fileName = [URL path];
        }
    }

    [openPanel close];

    if (fileName) {
        return [fileName UTF8String];
    }

    return NULL;
}
