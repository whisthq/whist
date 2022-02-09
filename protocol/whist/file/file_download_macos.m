#include <Foundation/Foundation.h>
#include "file_download.h"

void whist_file_download_notify_finished(const char* file_path) {
    // Bounce the downloads folder
    [[NSDistributedNotificationCenter defaultCenter]
        postNotificationName:@"com.apple.DownloadFileFinished"
                      object:[NSString stringWithUTF8String:file_path]];
}
