/*
 * General clipboard header file.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#if defined(_WIN32)
#pragma warning(disable : 4200)
#endif

#include <stdbool.h>

typedef enum ClipboardType {
    CLIPBOARD_NONE,
    CLIPBOARD_TEXT,
    CLIPBOARD_IMAGE,
    CLIPBOARD_FILES
} ClipboardType;

typedef struct ClipboardData {
    int size;
    ClipboardType type;
    char data[];
} ClipboardData;

typedef struct ClipboardFiles {
    int size;
    char* files[];
} ClipboardFiles;

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

WCHAR* lget_clipboard_directory();
WCHAR* lset_clipboard_directory();

char* get_clipboard_directory();
char* set_clipboard_directory();

#define LGET_CLIPBOARD (lget_clipboard_directory())
#define GET_CLIPBOARD (get_clipboard_directory())

#define LSET_CLIPBOARD (lset_clipboard_directory())
#define SET_CLIPBOARD (set_clipboard_directory())
#else
#define GET_CLIPBOARD "./get_clipboard"
#define SET_CLIPBOARD "./set_clipboard"
#endif

void initClipboard();

ClipboardData* GetClipboard();

void SetClipboard(ClipboardData* cb);

void StartTrackingClipboardUpdates();

bool hasClipboardUpdated();

// CLIPBOARD THREAD

typedef int SEND_FMSG(void* fmsg);

bool isUpdatingClipboard();
bool updateSetClipboard(ClipboardData* cb);
void initUpdateClipboard(SEND_FMSG* send_fmsg, char* server_ip);
void updateClipboard();
bool pendingUpdateClipboard();
void destroyUpdateClipboard();

#if defined(_WIN32)
#pragma warning(default : 4200)
#endif

#endif  // CLIPBOARD_H