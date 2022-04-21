#include <whist/core/whist.h>
#include "file_download.h"

#ifdef _WIN32

#include <ShlObj_core.h>

void whist_file_download_notify_finished(const char* filepath) {
    // Create an LPITEMIDLIST for the given filepath
    LPITEMIDLIST folder = ILCreateFromPathA(filepath);
    if (folder == NULL) {
        LOG_ERROR("ILCreateFromPathA failed");
        return;
    }

    // Open up and select that filepath
    HRESULT hr = SHOpenFolderAndSelectItems(folder, 0, NULL, 0);
    if (FAILED(hr)) {
        LOG_ERROR("SHOpenFolderAndSelectItems failed");
    }

    // Free the LPITEMIDLIST
    ILFree(folder);
}

#else

void whist_file_download_notify_finished(const char* filepath) {
    UNUSED(filepath);
    // Notification not implemented on X11 yet.
}

#endif
