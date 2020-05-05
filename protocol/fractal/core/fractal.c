/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock

void runcmd_nobuffer(const char* cmdline) {
    // Will run a command on the commandline, simple as that
#ifdef _WIN32
    // Windows makes this hard
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmd_buf[1000];

    if (strlen((const char*)cmdline) + 1 > sizeof(cmd_buf)) {
        mprintf("runcmd cmdline too long!\n");
        return;
    }

    memcpy(cmd_buf, cmdline, strlen((const char*)cmdline) + 1);

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

int runcmd(const char* cmdline, char** response) {
    if (response == NULL) {
        runcmd_nobuffer(cmdline);
        return 0;
    }

    FILE* pPipe;

    /* Run DIR so that it writes its output to a pipe. Open this
     * pipe with read text attribute so that we can read it
     * like a text file.
     */

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

    char* cmd = malloc(strlen(cmdline) + 128);
#ifdef _WIN32
    snprintf(cmd, strlen(cmdline) + 128, "%s 2>nul", cmdline);
#else
    snprintf(cmd, strlen(cmdline) + 128, "%s 2>/dev/null", cmdline);
#endif

    if ((pPipe = popen(cmd, "r")) == NULL) {
        return -1;
    }

    /* Read pipe until end of file, or an error occurs. */

    int current_len = 0;

    int max_len = 128;
    char* buffer = malloc(max_len);

    while (true) {
        char c = (char)fgetc(pPipe);
        if (current_len == max_len) {
            int next_max_len = 2 * max_len;
            char* next_buffer = malloc(next_max_len);

            memcpy(next_buffer, buffer, max_len);
            max_len = next_max_len;

            free(buffer);
            buffer = next_buffer;
        }

        if (c == EOF) {
            buffer[current_len] = '\0';
            break;
        } else {
            buffer[current_len] = c;
            current_len++;
        }
    }

    *response = buffer;

    /* Close pipe and print return value of pPipe. */
    if (feof(pPipe)) {
        return current_len;
    } else {
        printf("Error: Failed to read the pipe to the end.\n");
        return -1;
    }
}

char* get_ip() {
    static char ip[128];
    static bool already_obtained_ip = false;
    if (already_obtained_ip) {
        return ip;
    }

    char* buf;
    int len = runcmd("curl ipinfo.io", &buf);

    for (int i = 1; i < len; i++) {
        if (buf[i - 1] != '\n') {
            continue;
        }
        char* psBuffer = &buf[i];
#define IP_PREFIX_STRING "  \"ip\": \""
        if (strncmp(IP_PREFIX_STRING, psBuffer, sizeof(IP_PREFIX_STRING) - 1) ==
            0) {
            char* ip_start_string = psBuffer + sizeof(IP_PREFIX_STRING) - 1;
            for (int j = 0;; j++) {
                if (ip_start_string[j] == '\"') {
                    ip_start_string[j] = '\0';
                    break;
                }
            }
            memcpy(ip, ip_start_string, sizeof(ip));
        }
    }

    free(buf);

    already_obtained_ip = true;
    return ip;
}

void updateStatus(bool is_connected) {
    char json[1000];

    snprintf(json, sizeof(json),
             "{\
            \"ready\" : true\
    }");

    sendJSONPost("cube-celery-staging.herokuapp.com", "/vm/winlogonStatus",
                 json);

    snprintf(json, sizeof(json),
             "{\
            \"available\" : %s\
    }",
             is_connected ? "false" : "true");

    sendJSONPost("cube-celery-staging.herokuapp.com", "/vm/connectionStatus",
                 json);
}

int GetFmsgSize(struct FractalClientMessage* fmsg) {
    if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
        return sizeof(*fmsg);
    } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*fmsg) + fmsg->clipboard.size;
    } else {
        return sizeof(fmsg->type) + 40;
    }
}
