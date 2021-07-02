/**
 * Copyright Fractal Computers, Inc. 2020
 * @file win_clipboard.c
 * @brief This file contains the general clipboard functions for a shared
 *        client-server clipboard on Windows clients/servers.
============================
Usage
============================

GET_CLIPBOARD and SET_CLIPBOARD will return strings representing directories
important for getting and setting file clipboards. When GetClipboard() is called
and it returns a CLIPBOARD_FILES type, then GET_CLIPBOARD will be filled with
symlinks to the clipboard files. When SetClipboard(cb) is called and is given a
clipboard with a CLIPBOARD_FILES type, then the clipboard will be set to
whatever files are in the SET_CLIPBOARD directory.

LGET_CLIPBOARD and LSET_CLIPBOARD are the wide-character versions of these
strings, for use on windows OS's
*/

/*
============================
Includes
============================
*/

#ifdef _WIN32

#include <fractal/core/fractal.h>
#include "../utils/png.h"
#include "clipboard.h"

#include "shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")
#include "Knownfolders.h"
#include "Shellapi.h"
#include "shlobj_core.h"

/*
============================
Private Function Implementations
============================
*/

char* get_clipboard_directory() {
    /*
        Get the directory of the get clipboard cache

        Returns:
            (char*): the get clipboard directory string
    */

    static char buf[MAX_PATH];
    wcstombs(buf, lget_clipboard_directory(), sizeof(buf));
    return buf;
}
char* set_clipboard_directory() {
    /*
        Get the directory of the set clipboard cache

        Returns:
            (char*): the set clipboard directory string
    */

    static char buf[MAX_PATH];
    wcstombs(buf, lset_clipboard_directory(), sizeof(buf));
    return buf;
}

void unsafe_init_clipboard() {
    /*
        Initialize the clipboard by getting both the
        get clipboard and set clipboard caches
    */

    get_clipboard_directory();
    set_clipboard_directory();
}

void unsafe_destroy_clipboard() {}

WCHAR* lclipboard_directory() {
    /*
        Get the parent directory of the clipboard caches

        Returns:
            (WCHAR*): parent directory of the clipboard caches
    */

    static WCHAR* directory = NULL;
    if (directory == NULL) {
        static WCHAR sz_path[MAX_PATH];
        WCHAR* path;
        if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_ProgramData,
                                           CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, 0, &path))) {
            wcscpy(sz_path, path);
            CoTaskMemFree(path);
            PathAppendW(sz_path, L"FractalCache");
            if (!PathFileExistsW(sz_path)) {
                if (!CreateDirectoryW(sz_path, NULL)) {
                    LOG_ERROR("Could not create directory: %S (Error %d)", sz_path, GetLastError());
                    return NULL;
                }
            }
        } else {
            LOG_ERROR("Could not SHGetKnownFolderPath");
            return NULL;
        }
        directory = sz_path;
    }
    LOG_INFO("Directory: %S", directory);
    return directory;
}

WCHAR* lget_clipboard_directory() {
    /*
        Get the get clipboard cache directory

        Returns:
            (WCHAR*): the get clipboard cache directory
    */

    static WCHAR path[MAX_PATH];
    WCHAR* cb_dir = lclipboard_directory();
    wcscpy(path, cb_dir);
    PathAppendW(path, L"get_clipboard");
    if (!PathFileExistsW(path)) {
        if (!CreateDirectoryW(path, NULL)) {
            LOG_ERROR("Could not create directory: %S (Error %d)", path, GetLastError());
            return NULL;
        }
    }
    return path;
}

WCHAR* lset_clipboard_directory() {
    /*
        Get the set clipboard cache directory

        Returns:
            (WCHAR*): the set clipboard cache directory
    */

    static WCHAR path[MAX_PATH];
    WCHAR* cb_dir = lclipboard_directory();
    wcscpy(path, cb_dir);
    PathAppendW(path, L"set_clipboard");
    if (!PathFileExistsW(path)) {
        if (!CreateDirectoryW(path, NULL)) {
            LOG_ERROR("Could not create directory: %S (Error %d)", path, GetLastError());
            return NULL;
        }
    }
    LOG_INFO("SET PATH: %S", path);
    return path;
}

#define REPARSE_MOUNTPOINT_HEADER_SIZE 8

/**
 * @brief                          Struct for reparsing file mount point
 */
typedef struct {
    DWORD ReparseTag;
    DWORD ReparseDataLength;
    WORD Reserved;
    WORD ReparseTargetLength;
    WORD ReparseTargetMaximumLength;
    WORD Reserved1;
    WCHAR ReparseTarget[1];
} ReparseMountpointDataBuffer;

#include <winioctl.h>

bool create_junction(WCHAR* sz_junction, WCHAR* sz_path) {
    /*
        From filename `sz_junction`, create full path under `sz_path`

        Arguments:
            sz_junction (WCHAR*): filename of file to be created
            sz_path (WCHAR*): directory under which new file should be created

        Returns:
            (bool): true on success, false on failure
    */

    BYTE buf[sizeof(ReparseMountpointDataBuffer) + MAX_PATH * sizeof(WCHAR)];
    ReparseMountpointDataBuffer* reparse_buffer = (ReparseMountpointDataBuffer*)buf;
    WCHAR sz_target[MAX_PATH] = L"\\??\\";

    wcscat(sz_target, sz_path);
    wcscat(sz_target, L"\\");

    if (!CreateDirectoryW(sz_junction, NULL)) {
        LOG_ERROR("CreateDirectoryW Error: %d", GetLastError());
        return false;
    }

    // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
    HANDLE h_token = NULL;
    TOKEN_PRIVILEGES tp;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &h_token)) {
        LOG_ERROR("OpenProcessToken Error: %d", GetLastError());
        return false;
    }
    if (!LookupPrivilegeValueW(NULL, L"SeRestorePrivilege", &tp.Privileges[0].Luid)) {
        LOG_ERROR("LookupPrivilegeValueW Error: %d", GetLastError());
        return false;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(h_token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        LOG_ERROR("AdjustTokenPrivileges Error: %d", GetLastError());
        return false;
    }
    if (h_token) CloseHandle(h_token);
    // End Obtain SE_RESTORE_NAME privilege

    HANDLE h_dir = CreateFileW(sz_junction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                               FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h_dir == INVALID_HANDLE_VALUE) {
        LOG_ERROR("CreateFileW Error: %d", GetLastError());
        return false;
    }

    memset(buf, 0, sizeof(buf));
    reparse_buffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    int len = (int)wcslen(sz_target);
    wcscpy(reparse_buffer->ReparseTarget, sz_target);
    reparse_buffer->ReparseTargetMaximumLength = (WORD)len * sizeof(WCHAR);
    reparse_buffer->ReparseTargetLength = (WORD)(len - 1) * sizeof(WCHAR);
    reparse_buffer->ReparseDataLength = reparse_buffer->ReparseTargetLength + 12;

    DWORD dw_ret;
    if (!DeviceIoControl(h_dir, FSCTL_SET_REPARSE_POINT, reparse_buffer,
                         reparse_buffer->ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE, NULL,
                         0, &dw_ret, NULL)) {
        CloseHandle(h_dir);
        RemoveDirectoryW(sz_junction);

        LOG_ERROR("DeviceIoControl Error: %d", GetLastError());
        return false;
    }

    CloseHandle(h_dir);

    return true;
}

static int last_clipboard_sequence_number = -1;

bool unsafe_has_clipboard_updated() {
    /*
        Whether the Windows OS clipboard has updated

        Returns:
            (bool): true if clipboard has updated since last time this function
                was called, else false
    */

    bool has_updated = false;

    int new_clipboard_sequence_number = GetClipboardSequenceNumber();
    if (new_clipboard_sequence_number > last_clipboard_sequence_number) {
        if (should_preserve_local_clipboard()) {
            has_updated = true;
        }
        last_clipboard_sequence_number = new_clipboard_sequence_number;
    }
    return has_updated;
}

void unsafe_free_clipboard(ClipboardData* cb) {
    /*
        Free the clipboard memory

        Arguments:
            cb (ClipboardData*): the clipboard to be freed
    */
    deallocate_region(cb);
}

ClipboardData* unsafe_get_clipboard() {
    /*
        Get and return the current contents of the Windows clipboard

        Returns:
            (ClipboardData*): the clipboard data that has been read
                from the Windows OS clipboard
    */

    // We have to wait a bit after hasClipboardUpdated, before the clipboard actually updates
    fractal_sleep(15);

    ClipboardData* cb = allocate_region(sizeof(ClipboardData));

    cb->size = 0;
    cb->type = CLIPBOARD_NONE;

    if (!OpenClipboard(NULL)) {
        LOG_WARNING("Failed to open clipboard!");
        return cb;
    }

    int cf_types[] = {
        CF_TEXT, CF_DIB,
        // Not reading clipboard files right now
        // CF_HDROP,
    };

    int cf_type = -1;

    // Check for CF_TEXT/CF_FIB/CF_HDROP data, and if it exists, copy the data into cb
    for (int i = 0; i < (int)(sizeof(cf_types) / sizeof(cf_types[0])) && cf_type == -1; i++) {
        if (IsClipboardFormatAvailable(cf_types[i])) {
            HGLOBAL hglb = GetClipboardData(cf_types[i]);
            if (hglb != NULL) {
                LPTSTR lptstr = GlobalLock(hglb);
                if (lptstr != NULL) {
                    int data_size = (int)GlobalSize(hglb);

                    // Copy from global memory to Clipboard buffer
                    cb = realloc_region(cb, sizeof(ClipboardData) + data_size);
                    cb->size = data_size;
                    memcpy(cb->data, lptstr, data_size);
                    cf_type = cf_types[i];

                    // Don't forget to release the lock after you are done.
                    GlobalUnlock(hglb);
                } else {
                    LOG_WARNING("GlobalLock failed! (Type: %d) (Error: %d)", cf_types[i],
                                GetLastError());
                }
            }
        }
    }

    if (cf_type == -1) {
        LOG_WARNING("Clipboard not found");
        int ret = 0;
        int new_ret;
        while ((new_ret = EnumClipboardFormats(ret)) != 0) {
            char buf[1000];
            GetClipboardFormatNameA(new_ret, buf, sizeof(buf));
            LOG_INFO("Potential Format: %s", buf);
            ret = new_ret;
        }
    } else {
        switch (cf_type) {
            case CF_TEXT:
                // Read the contents of lptstr which just a pointer to the
                // string.
                cb->size = (int)strlen(cb->data);  // Kick null character out of cb->data
                LOG_INFO("CLIPBOARD STRING Received! Size: %d", cb->size);
                cb->type = CLIPBOARD_TEXT;
                break;
            case CF_DIB:
                LOG_INFO("Clipboard bitmap received! Size: %d", cb->size);

                // BMP will be 14 bytes larger than CF_DIB for the file header
                int bmp_size = cb->size + 14;

                // Windows clipboard saves bitmap data without file header
                // This will add file header and then convert from bmp to png
                char* bmp_data = allocate_region(bmp_size);
                *((char*)(&bmp_data[0])) = 'B';
                *((char*)(&bmp_data[1])) = 'M';
                *((int*)(&bmp_data[2])) = bmp_size;
                *((int*)(&bmp_data[6])) = 0;
                *((int*)(&bmp_data[10])) = 54;
                memcpy(bmp_data + 14, cb->data, cb->size);

                // convert BMP to PNG, and save it into the Clipboard
                // cb->size will already contain the max size of cb->data
                char* png;
                int png_size;
                if (bmp_to_png(bmp_data, bmp_size, &png, &png_size) != 0) {
                    LOG_ERROR("clipboard bmp to png conversion failed");
                    deallocate_region(bmp_data);
                    cb = realloc_region(cb, sizeof(ClipboardData));
                    cb->type = CLIPBOARD_NONE;
                    cb->size = 0;
                    break;
                }
                deallocate_region(bmp_data);

                // Reallocate for new png size
                deallocate_region(cb);
                cb = allocate_region(sizeof(ClipboardData) + png_size);
                // Copy png over
                cb->type = CLIPBOARD_IMAGE;
                cb->size = png_size;
                memcpy(cb->data, png, png_size);

                // Free png
                free_png(png);

                break;
            case CF_HDROP:
                LOG_WARNING("GetClipboard: FILE CLIPBOARD NOT BEING IMPLEMENTED");
                break;
                // we want to break, not return, because CloseClipboard() needs to be called
                // at the bottom
            default:
                LOG_WARNING("Clipboard type unknown: %d", cf_type);
                cb->type = CLIPBOARD_NONE;
                break;
        }
    }

    CloseClipboard();
    return cb;
}

HGLOBAL get_global_alloc(void* buf, int len, bool null_char) {
    /*
        Allocate space and copy buffer into allocated space

        Arguments:
            buf (void*): buffer to be copied into allocated space
            len (int): length of buffer being copied
            null_char (bool): whether to include a null character at the
                end of the allocated space

        Return:
            HGLOBAL: pointer to allocated memory
    */

    int alloc_len = null_char ? len + 1 : len;
    HGLOBAL h_mem = GlobalAlloc(GMEM_MOVEABLE, alloc_len);
    if (!h_mem) {
        LOG_ERROR("GlobalAlloc failed!");
        return h_mem;
    }
    // We use LPSTR instead of LPTSTR because LPSTR is always equivalent to (char*)
    //     while LPTSTR can have larger sized characters, leading to size overflows
    LPSTR lpstr = GlobalLock(h_mem);

    if (lpstr == NULL) {
        LOG_ERROR("get_global_alloc GlobalLock failed! Size %d", alloc_len);
        return h_mem;
    }

    memcpy(lpstr, buf, len);
    if (null_char) {
        memset(lpstr + len, 0, 1);
    }
    GlobalUnlock(h_mem);

    return h_mem;
}

void unsafe_set_clipboard(ClipboardData* cb) {
    /*
        Set the Windows clipboard to contain the data from `cb`

        Arguments:
            cb (ClipboardData*): the clipboard data to load into
                the Windows clipboard
    */

    if (cb->type == CLIPBOARD_NONE) {
        return;
    }

    int cf_type = -1;
    HGLOBAL h_mem = NULL;

    switch (cb->type) {
        case CLIPBOARD_TEXT:
            LOG_INFO("SetClipboard to Text with size: %d", cb->size);
            if (cb->size > 0) {
                cf_type = CF_TEXT;
                h_mem = get_global_alloc(cb->data, cb->size, true);  // add null char at end (true)
            }
            break;
        case CLIPBOARD_IMAGE:
            LOG_INFO("SetClipboard to Image with size %d", cb->size);
            if (cb->size > 0) {
                // Store max size on bmp_size, png_to_bmp will read this
                int bmp_size;
                char* bmp_data;
                if (png_to_bmp(cb->data, cb->size, &bmp_data, &bmp_size) != 0) {
                    LOG_ERROR("Clipboard image png -> bmp conversion failed");
                    return;
                }
                cf_type = CF_DIB;
                // Create a global allocation of the BMP
                h_mem = get_global_alloc(bmp_data + 14, bmp_size - 14,
                                         false);  // no null char at end (false)
                // Free the region that we received
                free_bmp(bmp_data);
            }
            break;
        case CLIPBOARD_FILES:
            LOG_INFO("SetClipboard to Files");

            LOG_WARNING("SetClipboard: FILE CLIPBOARD NOT BEING IMPLEMENTED");
            return;
            // we want to return, not break, because there is no content to place into the clipboard
        default:
            LOG_WARNING("Unknown clipboard type!");
            break;
    }

    if (cf_type != -1) {
        if (!OpenClipboard(NULL)) {
            GlobalFree(h_mem);
            return;
        }
        EmptyClipboard();
        if (!SetClipboardData(cf_type, h_mem)) {
            GlobalFree(h_mem);
            LOG_WARNING("Failed to SetClipboardData");
        }

        CloseClipboard();
    }
}

#endif  // _WIN32
