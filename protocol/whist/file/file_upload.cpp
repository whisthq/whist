#include <stddef.h>
extern "C" {
#include "file_upload.h"
#include <whist/core/whist.h>
}

#ifdef _WIN32

#include <ShObjIdl_core.h>

const char *whist_file_upload_get_picked_file() {
    IFileOpenDialog *p_file_open = NULL;
    IShellItem *p_item = NULL;
    PWSTR wstr_file_path = NULL;

    // Create the FileOpenDialog object.
    HRESULT hr = CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&p_file_open));
    if (FAILED(hr)) {
        LOG_ERROR("CoCreateInstance(FileOpenDialog, ...) Failed");
        return NULL;
    }

    // Show the file open dialog
    hr = p_file_open->Show(NULL);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        // The user canceled the dialog. Do not treat as an error.
        LOG_INFO("Open File Dialog closed");
        p_file_open->Release();
        return NULL;
    } else if (FAILED(hr)) {
        LOG_ERROR("p_file_open->Show Failed");
        p_file_open->Release();
        return NULL;
    }

    // Get the file open result from the dialog box
    hr = p_file_open->GetResult(&p_item);
    if (FAILED(hr)) {
        LOG_ERROR("p_file_open->GetResult Failed");
        p_file_open->Release();
        return NULL;
    }

    // Get the filename from the file open result
    hr = p_item->GetDisplayName(SIGDN_FILESYSPATH, &wstr_file_path);
    if (FAILED(hr)) {
        LOG_ERROR("p_item->GetDisplayName Failed");
        p_item->Release();
        p_file_open->Release();
        return NULL;
    }

    // Copy the filepath into a static buffer
    static char filepath_buffer[4096] = {0};
    wcstombs(filepath_buffer, wstr_file_path, sizeof(filepath_buffer));

    // Free everything we created
    CoTaskMemFree(wstr_file_path);
    p_item->Release();
    p_file_open->Release();

    // Return the filepath
    return filepath_buffer;
}

#else

const char *whist_file_upload_get_picked_file() {
    // File upload has not implemented on X11 yet.
    return NULL;
}

#endif
