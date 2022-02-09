#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include "file_upload.h"

const char* whist_file_upload_get_picked_file(void) {
    // Focus dialog window
    [NSApp activateIgnoringOtherApps:YES];

    // Open file dialog window with options
    NSOpenPanel* open_panel = [NSOpenPanel openPanel];
    [open_panel setLevel:NSFloatingWindowLevel];
    [open_panel setAllowsMultipleSelection:NO];
    [open_panel setCanChooseFiles:YES];
    [open_panel setCanChooseDirectories:NO];
    [open_panel setCanCreateDirectories:NO];
    [open_panel setMessage:@"Upload File to Whist"];
    [open_panel setPrompt:@"Upload"];

    // Choose last entry from url list after panel action
    NSString* file_name = nil;
    if ([open_panel runModal] == NSModalResponseOK) {
        for (NSURL* url in [open_panel URLs]) {
            file_name = [url path];
        }
    }

    [open_panel close];

    if (file_name) {
        return [file_name UTF8String];
    }

    return NULL;
}
