/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock

void runcmd(const char *cmdline) {
    // Will run a command on the commandline, simple as that
#ifdef _WIN32
    // Windows makes this hard
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmd_buf[1000];

    if (strlen((const char *)cmdline) + 1 > sizeof(cmd_buf)) {
        mprintf("runcmd cmdline too long!\n");
        return;
    }

    memcpy(cmd_buf, cmdline, strlen((const char *)cmdline) + 1);

    SetEnvironmentVariableW((LPCWSTR)L"UNISON", (LPCWSTR)L"./.unison");

    if (CreateProcessA(NULL, (LPSTR)cmd_buf, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#else
    // For Linux / MACOSX we make the system syscall
    system(cmdline);
#endif
}

int GetFmsgSize(struct FractalClientMessage *fmsg) {
    if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
        return sizeof(*fmsg);
    } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*fmsg) + fmsg->clipboard.size;
    } else {
        return sizeof(fmsg->type) + 40;
    }
}
