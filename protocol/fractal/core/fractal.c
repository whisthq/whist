/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock

#include <stdio.h>

#include "../utils/json.h"
#include "../utils/sysinfo.h"

// Print Memory Info

void PrintSystemInfo() {
    LOG_INFO("Hardware information:");

    PrintOSInfo();
    PrintModelInfo();
    PrintCPUInfo();
    PrintRAMInfo();
    PrintMonitors();
    PrintHardDriveInfo();
}

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
        LOG_WARNING("Failed to popen %s", cmd);
        free(cmd);
        return -1;
    }
    free(cmd);

    /* Read pipe until end of file, or an error occurs. */

    int current_len = 0;

    int max_len = 128;
    char* buffer = malloc(max_len);

    while (true) {
        char c = (char)fgetc(pPipe);
        if (current_len == max_len) {
            int next_max_len = 2 * max_len;
            char* new_buffer = realloc(buffer, next_max_len);
            if (new_buffer == NULL) {
                LOG_ERROR("Realloc from %d to %d failed!", max_len,
                          next_max_len);
                buffer[max_len] = '\0';
                break;
            } else {
                buffer = new_buffer;
                max_len = next_max_len;
            }
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
        LOG_WARNING("Error: Failed to read the pipe to the end.\n");
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
    len;

    json_t json;
    if (!parse_json(buf, &json)) {
        LOG_WARNING("curl ipinfo.io did not return an IP: %s", buf);
        return NULL;
    }
    kv_pair_t* kv = get_kv(&json, "ip");

    memcpy(ip, kv->str_value, sizeof(ip));

    free_json(json);

    already_obtained_ip = true;
    return ip;
}

static char* branch = NULL;
static bool is_dev;
static bool already_obtained_vm_type = false;

char* get_branch() {
    is_dev_vm();
    return branch;
}

bool is_dev_vm() {
    if (already_obtained_vm_type) {
        return is_dev;
    }

    char buf[4800];
    size_t len = sizeof(buf);

    LOG_INFO("GETTING JSON");

    if (!SendJSONGet(PRODUCTION_HOST, "/vm/isDev", buf, len)) {
        return true;
    }

    char* json_str = NULL;
    for (size_t i = 0; i < len - 4; i++) {
        if (memcmp(buf + i, "\r\n\r\n", 4) == 0) {
            json_str = buf + i + 4;
        }
    }

    json_t json;
    if (!parse_json(json_str, &json)) {
        LOG_WARNING("Failed to parse JSON from /vm/isDev");
        already_obtained_vm_type = true;
        is_dev = true;
        return is_dev;
    }

    kv_pair_t* dev_value = get_kv(&json, "dev");
    kv_pair_t* branch_value = get_kv(&json, "branch");
    if (dev_value && branch_value) {
        if (dev_value->type != JSON_BOOL) {
            return false;
        }

        is_dev = dev_value->bool_value;
        branch = clone(branch_value->str_value);

        LOG_INFO("Is Dev? %s", dev_value->bool_value ? "true" : "false");
        LOG_INFO("Branch: %s", branch);

        free_json(json);

        already_obtained_vm_type = true;
        return is_dev;
    } else {
        free_json(json);
        return true;
    }
}

int GetFmsgSize(FractalClientMessage* fmsg) {
    if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
        return sizeof(*fmsg);
    } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*fmsg) + fmsg->clipboard.size;
    } else {
        return sizeof(fmsg->type) + 40;
    }
}
