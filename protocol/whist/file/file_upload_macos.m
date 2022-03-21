#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include "file_upload.h"
#include "file_synchronizer.h"

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

// TODO: Lots of repeated code - should be one function with flags!
const char* whist_multi_file_upload_get_picked_file(void) {
    // Focus dialog window
    [NSApp activateIgnoringOtherApps:YES];

    // Open file dialog window with options
    NSOpenPanel* open_panel = [NSOpenPanel openPanel];
    [open_panel setLevel:NSFloatingWindowLevel];
    [open_panel setAllowsMultipleSelection:YES];
    [open_panel setCanChooseFiles:YES];
    [open_panel setCanChooseDirectories:NO];
    [open_panel setCanCreateDirectories:NO];
    [open_panel setMessage:@"Upload Files to Whist"];
    [open_panel setPrompt:@"Upload"];
    FileEventInfo upload_event_info;

    // Choose last entry from url list after panel action
    NSString* file_name = nil;
    int group_size;
    if ([open_panel runModal] == NSModalResponseOK) {
        group_size = [[open_panel URLs] count];
        upload_event_info.group_size = group_size;
        for (NSURL* url in [open_panel URLs]) {
            file_name = [url path];
            file_synchronizer_set_file_reading_basic_metadata([file_name UTF8String],
                                                              FILE_TRANSFER_SERVER_UPLOAD,
                                                              &upload_event_info);
        }
    }

    [open_panel close];

    if (file_name) {
        return [file_name UTF8String];
    }

    return NULL;
}
