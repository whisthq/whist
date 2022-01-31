/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file clipboard_internal.h
 * @brief Internal functions for OS-dependent clipboard handling.
 */
#ifndef WHIST_CLIPBOARD_CLIPBOARD_INTERNAL_H
#define WHIST_CLIPBOARD_CLIPBOARD_INTERNAL_H

#include <stdbool.h>
#include "clipboard.h"

#ifdef _WIN32
#include <windows.h>

WCHAR* lget_os_clipboard_directory(void);
WCHAR* lset_os_clipboard_directory(void);
char* get_os_clipboard_directory(void);
char* set_os_clipboard_directory(void);
#define LGET_OS_CLIPBOARD (lget_os_clipboard_directory())
#define GET_OS_CLIPBOARD (get_cos_lipboard_directory())
#define LSET_OS_CLIPBOARD (lset_os_clipboard_directory())
#define SET_OS_CLIPBOARD (set_cos_lipboard_directory())
// If we are on Windows, MAX_PATH is defined; otherwise, we need to use PATH_MAX.
#define PATH_MAXLEN MAX_PATH - 1
#else
#define GET_OS_CLIPBOARD "./get_os_clipboard"
#define SET_OS_CLIPBOARD "./set_os_clipboard"
#define PATH_MAXLEN PATH_MAX - 1
#endif

// These clipboard primitives are defined in {x11,win,mac}_clipboard.c
// CMake will only link one of those C files, depending on the OS

void unsafe_init_clipboard(void);
ClipboardData* unsafe_get_os_clipboard(void);
void unsafe_set_os_clipboard(ClipboardData* cb);
void unsafe_free_clipboard_buffer(ClipboardData* cb);
bool unsafe_has_os_clipboard_updated(void);
void unsafe_destroy_clipboard(void);

#endif /* WHIST_CLIPBOARD_CLIPBOARD_INTERNAL_H */
