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

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "../core/fractal.h"
#include "clipboard.h"

bool StartTrackingClipboardUpdates();

#ifdef _WIN32
char* get_clipboard_directory() {
    static char buf[MAX_PATH];
    wcstombs(buf, lget_clipboard_directory(), sizeof(buf));
    return buf;
}
char* set_clipboard_directory() {
    static char buf[MAX_PATH];
    wcstombs(buf, lset_clipboard_directory(), sizeof(buf));
    return buf;
}
#endif

void initClipboard() {
    get_clipboard_directory();
    set_clipboard_directory();
    StartTrackingClipboardUpdates();
}

#ifdef _WIN32
#include "shlwapi.h"
#pragma comment(lib, "Shlwapi.lib")
#include "Knownfolders.h"
#include "Shellapi.h"
#include "shlobj_core.h"

WCHAR* lclipboard_directory() {
    static WCHAR* directory = NULL;
    if (directory == NULL) {
        static WCHAR szPath[MAX_PATH];
        WCHAR* path;
        if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_ProgramData,
                                           CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, 0, &path))) {
            wcscpy(szPath, path);
            CoTaskMemFree(path);
            PathAppendW(szPath, L"FractalCache");
            if (!PathFileExistsW(szPath)) {
                if (!CreateDirectoryW(szPath, NULL)) {
                    LOG_ERROR("Could not create directory: %S (Error %d)", szPath, GetLastError());
                    return NULL;
                }
            }
        } else {
            LOG_ERROR("Could not SHGetKnownFolderPath");
            return NULL;
        }
        directory = szPath;
    }
    LOG_INFO("Directory: %S", directory);
    return directory;
}

WCHAR* lget_clipboard_directory() {
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

typedef struct {
    DWORD ReparseTag;
    DWORD ReparseDataLength;
    WORD Reserved;
    WORD ReparseTargetLength;
    WORD ReparseTargetMaximumLength;
    WORD Reserved1;
    WCHAR ReparseTarget[1];
} REPARSE_MOUNTPOINT_DATA_BUFFER, *PREPARSE_MOUNTPOINT_DATA_BUFFER;

#include <winioctl.h>

bool CreateJunction(WCHAR* szJunction, WCHAR* szPath);

bool CreateJunction(WCHAR* szJunction, WCHAR* szPath) {
    BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
    REPARSE_MOUNTPOINT_DATA_BUFFER* ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER*)buf;
    WCHAR szTarget[MAX_PATH] = L"\\??\\";

    wcscat(szTarget, szPath);
    wcscat(szTarget, L"\\");

    if (!CreateDirectoryW(szJunction, NULL)) {
        LOG_ERROR("CreateDirectoryW Error: %d", GetLastError());
        return false;
    }

    // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        LOG_ERROR("OpenProcessToken Error: %d", GetLastError());
        return false;
    }
    if (!LookupPrivilegeValueW(NULL, L"SeRestorePrivilege", &tp.Privileges[0].Luid)) {
        LOG_ERROR("LookupPrivilegeValueW Error: %d", GetLastError());
        return false;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        LOG_ERROR("AdjustTokenPrivileges Error: %d", GetLastError());
        return false;
    }
    if (hToken) CloseHandle(hToken);
    // End Obtain SE_RESTORE_NAME privilege

    HANDLE hDir = CreateFileW(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                              FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) {
        LOG_ERROR("CreateFileW Error: %d", GetLastError());
        return false;
    }

    memset(buf, 0, sizeof(buf));
    ReparseBuffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    int len = (int)wcslen(szTarget);
    wcscpy(ReparseBuffer->ReparseTarget, szTarget);
    ReparseBuffer->ReparseTargetMaximumLength = (WORD)len * sizeof(WCHAR);
    ReparseBuffer->ReparseTargetLength = (WORD)(len - 1) * sizeof(WCHAR);
    ReparseBuffer->ReparseDataLength = ReparseBuffer->ReparseTargetLength + 12;

    DWORD dwRet;
    if (!DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, ReparseBuffer,
                         ReparseBuffer->ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0,
                         &dwRet, NULL)) {
        CloseHandle(hDir);
        RemoveDirectoryW(szJunction);

        LOG_ERROR("DeviceIoControl Error: %d", GetLastError());
        return false;
    }

    CloseHandle(hDir);

    return true;
}

#endif

static int last_clipboard_sequence_number = -1;

static char clipboard_buf[9000000];

bool StartTrackingClipboardUpdates() {
    last_clipboard_sequence_number = GetClipboardSequenceNumber();
    return true;
}

bool hasClipboardUpdated() {
    bool hasUpdated = false;

    int new_clipboard_sequence_number = GetClipboardSequenceNumber();
    if (new_clipboard_sequence_number > last_clipboard_sequence_number) {
        hasUpdated = true;
        last_clipboard_sequence_number = new_clipboard_sequence_number;
    }
    return hasUpdated;
}

ClipboardData* GetClipboard() {
    // We have to wait a bit after hasClipboardUpdated, before the clipboard actually updates 
    SDL_Delay(15);

    ClipboardData* cb = (ClipboardData*)clipboard_buf;

    cb->size = 0;
    cb->type = CLIPBOARD_NONE;

    if (!OpenClipboard(NULL)) {
        LOG_WARNING("Failed to open clipboard!");
        return cb;
    }

    int cf_types[] = {
        CF_TEXT,
        CF_DIB,
        CF_HDROP,
    };

    int cf_type = -1;

    for (int i = 0; i < sizeof(cf_types) / sizeof(cf_types[0]) && cf_type == -1; i++) {
        if (IsClipboardFormatAvailable(cf_types[i])) {
            HGLOBAL hglb = GetClipboardData(cf_types[i]);
            if (hglb != NULL) {
                LPTSTR lptstr = GlobalLock(hglb);
                if (lptstr != NULL) {
                    int data_size = (int)GlobalSize(hglb);
                    if (data_size < sizeof(clipboard_buf)) {
                        cb->size = data_size;
                        memcpy(cb->data, lptstr, data_size);
                        cf_type = cf_types[i];
                    } else {
                        LOG_WARNING("Could not copy, clipboard too large! %d bytes", data_size);
                    }

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
                LOG_INFO("CLIPBOARD STRING Received! Size: %d", cb->size);
                cb->type = CLIPBOARD_TEXT;
                break;
            case CF_DIB:
                LOG_INFO("Clipboard bitmap received! Size: %d", cb->size);
                cb->type = CLIPBOARD_IMAGE;
                break;
            case CF_HDROP:
                LOG_INFO("Hdrop! Size: %d", cb->size);
                DROPFILES drop;
                memcpy(&drop, cb->data, sizeof(DROPFILES));

                LOG_INFO("Drop pFiles: %d", drop.pFiles);

                // Begin copy to clipboard
                WCHAR* filename = (WCHAR*)(cb->data + sizeof(drop));

                SHFILEOPSTRUCTW sh = {0};
                sh.wFunc = FO_DELETE;
                sh.fFlags = 0;
                sh.fFlags |= FOF_SILENT;
                sh.fFlags |= FOF_NOCONFIRMMKDIR;
                sh.fFlags |= FOF_NOCONFIRMATION;
                sh.fFlags |= FOF_WANTMAPPINGHANDLE;
                sh.fFlags |= FOF_NOERRORUI;
                sh.pFrom = LGET_CLIPBOARD;

                clock t;
                StartTimer(&t);
                // SHFileOperationW( &sh );

                WIN32_FIND_DATAW data;

                WCHAR first_file_path[MAX_PATH] = L"";
                wcscat(first_file_path, LGET_CLIPBOARD);
                wcscat(first_file_path, L"\\*");

                HANDLE hFind = FindFirstFileW(first_file_path, &data);
                if (hFind != INVALID_HANDLE_VALUE) {
                    WCHAR* ignore1 = L".";
                    WCHAR* ignore2 = L"..";

                    do {
                        if (wcscmp(data.cFileName, ignore1) == 0 ||
                            wcscmp(data.cFileName, ignore2) == 0) {
                            continue;
                        }

                        WCHAR cur_filename[MAX_PATH] = L"";
                        wcscat((wchar_t*)cur_filename, LGET_CLIPBOARD);
                        wcscat((wchar_t*)cur_filename, L"\\");
                        wcscat((wchar_t*)cur_filename, data.cFileName);

                        LOG_INFO("Deleting %S...", cur_filename);

                        DWORD fileattributes = GetFileAttributesW((LPCWSTR)cur_filename);
                        if (fileattributes == INVALID_FILE_ATTRIBUTES) {
                            LOG_WARNING("GetFileAttributesW Error: %d", GetLastError());
                        }

                        if (fileattributes & FILE_ATTRIBUTE_DIRECTORY) {
                            if (!RemoveDirectoryW((LPCWSTR)cur_filename)) {
                                LOG_WARNING("Delete Folder Error: %d", GetLastError());
                            }
                        } else {
                            if (!DeleteFileW((LPCWSTR)cur_filename)) {
                                LOG_WARNING("Delete Folder Error: %d", GetLastError());
                            }
                        }
                    } while (FindNextFileW(hFind, &data));
                    FindClose(hFind);
                }

                if (!CreateDirectoryW(LGET_CLIPBOARD, NULL)) {
                    int err = GetLastError();
                    if (err != ERROR_ALREADY_EXISTS) {
                        LOG_WARNING("Could not create directory: %S (Error %d)", LGET_CLIPBOARD,
                                    GetLastError());
                        break;
                    }
                }

                // Go through filenames
                while (*filename != L'\0') {
                    WCHAR* fileending = PathFindFileNameW(filename);
                    DWORD fileattributes = GetFileAttributesW(filename);

                    WCHAR target_file[MAX_PATH] = L"";
                    wcscat(target_file, LGET_CLIPBOARD);
                    wcscat(target_file, L"\\");
                    wcsncat(target_file, fileending, sizeof(target_file));

                    LOG_INFO("Target: %S", target_file);
                    LOG_INFO("Src: %S", filename);

                    if (fileattributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (!CreateJunction(target_file, filename)) {
                            LOG_WARNING("CreateJunction Error: %d", GetLastError());
                        }
                    } else {
                        if (!CreateHardLinkW(target_file, filename, 0)) {
                            LOG_WARNING("CreateHardLinkW Error: %d", GetLastError());
                        }
                    }

                    LOG_INFO("TARGET FILENAME: %S", target_file);
                    LOG_INFO("FILENAME: %S", filename);
                    LOG_INFO("FILENAME ENDING: %S", fileending);

                    filename += wcslen(filename) + 1;
                }
                LOG_INFO("Time: %f", GetTimer(t));

                cb->type = CLIPBOARD_FILES;
                cb->size = 0;

                break;
            default:
                LOG_WARNING("Clipboard type unknown: %d", cf_type);
                cb->type = CLIPBOARD_NONE;
                break;
        }
    }

    CloseClipboard();
    return cb;
}

HGLOBAL getGlobalAlloc(void* buf, int len) {
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (!hMem) {
        LOG_ERROR("GlobalAlloc failed!");
        return hMem;
    }
    LPTSTR lptstr = GlobalLock(hMem);

    if (lptstr == NULL) {
        LOG_ERROR("getGlobalAlloc GlobalLock failed! Size %d", len);
        return hMem;
    }

    memcpy(lptstr, buf, len);
    GlobalUnlock(hMem);

    return hMem;
}

void SetClipboard(ClipboardData* cb) {
    if (cb->type == CLIPBOARD_NONE) {
        return;
    }

    int cf_type = -1;
    HGLOBAL hMem = NULL;

    switch (cb->type) {
        case CLIPBOARD_TEXT:
            LOG_INFO("SetClipboard to Text: %s", cb->data);
            if (cb->size > 0) {
                cf_type = CF_TEXT;
                hMem = getGlobalAlloc(cb->data, cb->size);
            }
            break;
        case CLIPBOARD_IMAGE:
            LOG_INFO("SetClipboard to Image with size %d", cb->size);
            if (cb->size > 0) {
                cf_type = CF_DIB;
                hMem = getGlobalAlloc(cb->data, cb->size);
            }
            break;
        case CLIPBOARD_FILES:
            LOG_INFO("SetClipboard to Files");

            WCHAR first_file_path[MAX_PATH] = L"";
            wcscat(first_file_path, LSET_CLIPBOARD);
            wcscat(first_file_path, L"\\*");

            WIN32_FIND_DATAW data;
            HANDLE hFind = FindFirstFileW(first_file_path, &data);

            DROPFILES* drop = (DROPFILES*)clipboard_buf;
            memset(drop, 0, sizeof(DROPFILES));
            WCHAR* file_ptr = (WCHAR*)(clipboard_buf + sizeof(DROPFILES));
            int total_len = sizeof(DROPFILES);
            int num_files = 0;
            drop->pFiles = total_len;
            drop->fWide = true;
            drop->fNC = false;

            WCHAR file_prefix[MAX_PATH] = L"";
            wcscat(file_prefix, LSET_CLIPBOARD);
            wcscat(file_prefix, L"\\");

            int file_prefix_len = (int)wcslen(file_prefix);

            if (hFind != INVALID_HANDLE_VALUE) {
                WCHAR* ignore1 = L".";
                WCHAR* ignore2 = L"..";

                do {
                    if (wcscmp(data.cFileName, ignore1) == 0 ||
                        wcscmp(data.cFileName, ignore2) == 0) {
                        continue;
                    }

                    memcpy(file_ptr, file_prefix, sizeof(WCHAR) * file_prefix_len);
                    // file_ptr moves in terms of WCHAR
                    file_ptr += file_prefix_len;
                    // total_len moves in terms of bytes
                    total_len += sizeof(WCHAR) * file_prefix_len;

                    int len = (int)wcslen(data.cFileName) + 1;  // Including null terminator

                    LOG_INFO("FILENAME: %S", data.cFileName);

                    memcpy(file_ptr, data.cFileName, sizeof(WCHAR) * len);
                    file_ptr += len;
                    total_len += sizeof(WCHAR) * len;

                    num_files++;
                } while (FindNextFileW(hFind, &data));
                FindClose(hFind);
            }

            LOG_INFO("File: %S", (char*)drop + drop->pFiles);

            *file_ptr = L'\0';
            total_len += sizeof(L'\0');

            cf_type = CF_HDROP;
            hMem = getGlobalAlloc(drop, total_len);

            break;
        default:
            LOG_WARNING("Unknown clipboard type!");
            break;
    }

    if (cf_type != -1) {
        if (!OpenClipboard(NULL)) {
            GlobalFree(hMem);
            return;
        }
        EmptyClipboard();
        if (!SetClipboardData(cf_type, hMem)) {
            GlobalFree(hMem);
            LOG_WARNING("Failed to SetClipboardData");
        }

        CloseClipboard();
    }

    // Update the status so that this specific update doesn't count
    hasClipboardUpdated();
}
