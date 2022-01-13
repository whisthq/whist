#include <whist/core/whist.h>
#include "file_download.h"

void whist_file_download_notify_finished(const char* filename) {
    UNUSED(filename);
    // Notification not implemented on Windows/X11 yet.
    // TODO: For Windows, use SHOpenFolderAndSelectItems
    // to match what Chrome does to show item in folder.
}
