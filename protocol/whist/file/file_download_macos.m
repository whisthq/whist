#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include "file_download.h"

void whist_file_download_notify_finished(const char* file_path) {
    // Bounce the downloads folder
    [[NSDistributedNotificationCenter defaultCenter]
        postNotificationName:@"com.apple.DownloadFileFinished"
                      object:[NSString stringWithUTF8String:file_path]];
}



// TODO: MOVE THIS TO A SEPARATE FILE!
const char *whist_file_upload_get_picked_file(void) {
        [NSApp activateIgnoringOtherApps:YES];

        NSOpenPanel *openPanel = [NSOpenPanel openPanel];
        //[openPanel setLevel:NSFloatingWindowLevel];
        [openPanel setAllowsMultipleSelection:NO];
        [openPanel setCanChooseFiles:YES];
        [openPanel setCanChooseDirectories:NO];
        [openPanel setCanCreateDirectories:NO];
        //[openPanel setMessage:title];
        //[openPanel setPrompt:buttonTitle];

        NSString* fileName = nil;
        if ([openPanel runModal] == NSModalResponseOK)
        {
            for( NSURL* URL in [openPanel URLs])
            {
                fileName = [URL path];
            }
        }
        [openPanel close];
        return [fileName UTF8String];
}
