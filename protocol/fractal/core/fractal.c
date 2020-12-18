/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock

#include <ctype.h>
#include <stdio.h>

#include "../utils/sysinfo.h"

#define UNUSED(x) (void)(x)
char sentry_environment[FRACTAL_ENVIRONMENT_MAXLEN];
bool using_sentry = false;

// Print Memory Info

int multithreaded_print_system_info(void* opaque) {
    UNUSED(opaque);

    LOG_INFO("Hardware information:");

    print_os_info();
    print_model_info();
    print_cpu_info();
    print_ram_info();
    print_monitors();
    print_hard_drive_info();

    return 0;
}

void print_system_info() {
    SDL_Thread* sysinfo_thread =
        SDL_CreateThread(multithreaded_print_system_info, "print_system_info", NULL);
    SDL_DetachThread(sysinfo_thread);
}

typedef struct DynamicBuffer {
    int size;
    int capacity;
    char* buf;
} DynamicBuffer;

DynamicBuffer* init_dynamic_buffer() {
    DynamicBuffer* db = malloc(sizeof(DynamicBuffer));
    db->size = 0;
    db->capacity = 128;
    db->buf = malloc(db->capacity);
    if (!db->buf) {
        LOG_ERROR("Could not malloc size %d!", db->capacity);
        SDL_Delay(50);
        exit(-1);
    }
    return db;
}

void resize_dynamic_buffer(DynamicBuffer* db, int new_size) {
    if (new_size > db->capacity) {
        int new_capacity = new_size * 2;
        char* new_buffer = realloc(db->buf, new_capacity);
        if (!new_buffer) {
            LOG_ERROR("Could not realloc from %d to %d!", db->capacity, new_capacity);
            SDL_Delay(50);
            exit(-1);
        } else {
            db->capacity = new_capacity;
            db->size = new_size;
            db->buf = new_buffer;
        }
    } else {
        db->size = new_size;
    }
}

void free_dynamic_buffer(DynamicBuffer* db) {
    free(db->buf);
    free(db);
}

int runcmd(const char* cmdline, char** response) {
#ifdef _WIN32
    HANDLE h_child_std_in_rd = NULL;
    HANDLE h_child_std_in_wr = NULL;
    HANDLE h_child_std_out_rd = NULL;
    HANDLE h_child_std_out_wr = NULL;

    SECURITY_ATTRIBUTES sa_attr;

    // Set the bInheritHandle flag so pipe handles are inherited.

    sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa_attr.bInheritHandle = TRUE;
    sa_attr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.

    if (response) {
        if (!CreatePipe(&h_child_std_out_rd, &h_child_std_out_wr, &sa_attr, 0)) {
            LOG_ERROR("StdoutRd CreatePipe failed");
            *response = NULL;
            return -1;
        }
        if (!SetHandleInformation(h_child_std_out_rd, HANDLE_FLAG_INHERIT, 0)) {
            LOG_ERROR("Stdout SetHandleInformation failed");
            *response = NULL;
            return -1;
        }
        if (!CreatePipe(&h_child_std_in_rd, &h_child_std_in_wr, &sa_attr, 0)) {
            LOG_ERROR("Stdin CreatePipe failed");
            *response = NULL;
            return -1;
        }
        if (!SetHandleInformation(h_child_std_in_wr, HANDLE_FLAG_INHERIT, 0)) {
            LOG_ERROR("Stdin SetHandleInformation failed");
            *response = NULL;
            return -1;
        }
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (response) {
        si.hStdError = h_child_std_out_wr;
        si.hStdOutput = h_child_std_out_wr;
        si.hStdInput = h_child_std_in_rd;
        si.dwFlags |= STARTF_USESTDHANDLES;
    }
    ZeroMemory(&pi, sizeof(pi));

    char cmd_buf[1000];

    while (cmdline[0] == ' ') {
        cmdline++;
    }

    if (strlen((const char*)cmdline) + 1 > sizeof(cmd_buf)) {
        LOG_WARNING("runcmd cmdline too long!");
        if (response) {
            *response = NULL;
        }
        return -1;
    }

    memcpy(cmd_buf, cmdline, strlen((const char*)cmdline) + 1);

    if (CreateProcessA(NULL, (LPSTR)cmd_buf, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si,
                       &pi)) {
    } else {
        LOG_ERROR("CreateProcessA failed!");
        if (response) {
            *response = NULL;
        }
        return -1;
    }

    if (response) {
        CloseHandle(h_child_std_out_wr);
        CloseHandle(h_child_std_in_rd);

        CloseHandle(h_child_std_in_wr);

        DWORD dw_read;
        CHAR ch_buf[2048];
        BOOL b_success = FALSE;

        DynamicBuffer* db = init_dynamic_buffer();
        for (;;) {
            b_success = ReadFile(h_child_std_out_rd, ch_buf, sizeof(ch_buf), &dw_read, NULL);
            if (!b_success || dw_read == 0) break;

            int original_size = db->size;
            resize_dynamic_buffer(db, original_size + dw_read);
            memcpy(db->buf + original_size, ch_buf, dw_read);
            if (!b_success) break;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        resize_dynamic_buffer(db, db->size + 1);
        db->buf[db->size] = '\0';
        resize_dynamic_buffer(db, db->size - 1);
        int size = db->size;
        *response = db->buf;
        free(db);
        return size;
    } else {
        CloseHandle(h_child_std_out_wr);
        CloseHandle(h_child_std_in_rd);
        CloseHandle(h_child_std_in_wr);

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 0;
    }
#else
    if (response == NULL) {
        system(cmdline);
        return 0;
    } else {
        FILE* p_pipe;

        /* Run DIR so that it writes its output to a pipe. Open this
         * pipe with read text attribute so that we can read it
         * like a text file.
         */

        char* cmd = malloc(strlen(cmdline) + 128);
        snprintf(cmd, strlen(cmdline) + 128, "%s 2>/dev/null", cmdline);

        if ((p_pipe = popen(cmd, "r")) == NULL) {
            LOG_WARNING("Failed to popen %s", cmd);
            free(cmd);
            return -1;
        }
        free(cmd);

        /* Read pipe until end of file, or an error occurs. */

        int current_len = 0;
        DynamicBuffer* db = init_dynamic_buffer();

        while (true) {
            char c = (char)fgetc(p_pipe);

            resize_dynamic_buffer(db, current_len + 1);

            if (c == EOF) {
                db->buf[current_len] = '\0';
                break;
            } else {
                db->buf[current_len] = c;
                current_len++;
            }
        }

        *response = db->buf;
        free(db);

        /* Close pipe and print return value of pPipe. */
        if (feof(p_pipe)) {
            return current_len;
        } else {
            LOG_WARNING("Error: Failed to read the pipe to the end.");
            *response = NULL;
            return -1;
        }
    }
#endif
}

bool read_hexadecimal_private_key(char* hex_string, char* binary_private_key,
                                  char* hex_private_key) {
    // It looks wasteful to convert from string to binary and back, but we need
    // to validate the hex string anyways, and it's easier to see exactly the
    // format in which we're storing it (big-endian).

    if (strlen(hex_string) != 32) {
        return false;
    }
    for (int i = 0; i < 16; i++) {
        if (!isxdigit(hex_string[2 * i]) || !isxdigit(hex_string[2 * i + 1])) {
            return false;
        }
        sscanf(&hex_string[2 * i], "%2hhx", &(binary_private_key[i]));
    }

    snprintf(hex_private_key, 33, "%08X%08X%08X%08X", htonl(*((uint32_t*)(binary_private_key))),
             htonl(*((uint32_t*)(binary_private_key + 4))),
             htonl(*((uint32_t*)(binary_private_key + 8))),
             htonl(*((uint32_t*)(binary_private_key + 12))));

    return true;
}

int get_fmsg_size(FractalClientMessage* fmsg) {
    if (fmsg->type == MESSAGE_KEYBOARD_STATE || fmsg->type == MESSAGE_DISCOVERY_REQUEST) {
        return sizeof(*fmsg);
    } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*fmsg) + fmsg->clipboard.size;
    } else {
        // Send small fmsg's when we don't need unnecessarily large ones
        return sizeof(fmsg->type) + 40;
    }
}
