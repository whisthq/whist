/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock
#include "../utils/logging.h"

#include <ctype.h>
#include <stdio.h>

#include "../utils/sysinfo.h"

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
    DynamicBuffer* db = safe_malloc(sizeof(DynamicBuffer));
    db->size = 0;
    db->capacity = 128;
    db->buf = safe_malloc(db->capacity);
    if (!db->buf) {
        LOG_FATAL("Could not safe_malloc size %d!", db->capacity);
    }
    return db;
}

void resize_dynamic_buffer(DynamicBuffer* db, int new_size) {
    if (new_size > db->capacity) {
        int new_capacity = new_size * 2;
        char* new_buffer = realloc(db->buf, new_capacity);
        if (!new_buffer) {
            LOG_FATAL("Could not realloc from %d to %d!", db->capacity, new_capacity);
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

        char* cmd = safe_malloc(strlen(cmdline) + 128);
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

    snprintf(hex_private_key, 33, "%08X%08X%08X%08X",
             (unsigned int)htonl(*((uint32_t*)(binary_private_key))),
             (unsigned int)htonl(*((uint32_t*)(binary_private_key + 4))),
             (unsigned int)htonl(*((uint32_t*)(binary_private_key + 8))),
             (unsigned int)htonl(*((uint32_t*)(binary_private_key + 12))));

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

void terminate_protocol() {
    LOG_INFO("Terminating Protocol");
    destroy_logger();
    print_stacktrace();
    exit(-1);
}

void* safe_malloc(size_t size) {
    void* ret = malloc(size);
    if (ret == NULL) {
        LOG_FATAL("Malloc of size %d failed!", size);
    } else {
        return ret;
    }
}

SDL_mutex* safe_SDL_CreateMutex() {  // NOLINT(readability-identifier-naming)
    SDL_mutex* ret = SDL_CreateMutex();
    if (ret == NULL) {
        LOG_FATAL("Failed to safe_SDL_CreateMutex! %s", SDL_GetError());
    }
    return ret;
}

void safe_SDL_LockMutex(SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    if (SDL_LockMutex(mutex) < 0) {
        LOG_FATAL("Failed to safe_SDL_LockMutex! %s", SDL_GetError());
    }
}

int safe_SDL_TryLockMutex(SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    int status = SDL_TryLockMutex(mutex);
    if (status == 0 && status == SDL_MUTEX_TIMEDOUT) {
        return status;
    } else {
        LOG_FATAL("Failed to safe_SDL_LockMutex! %s", SDL_GetError());
    }
}

void safe_SDL_UnlockMutex(SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    if (SDL_UnlockMutex(mutex) < 0) {
        LOG_FATAL("Failed to safe_SDL_UnlockMutex! %s", SDL_GetError());
    }
}

void safe_SDL_CondWait(SDL_cond* cond, SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    if (SDL_CondWait(cond, mutex) < 0) {
        LOG_FATAL("Failed to safe_SDL_CondWait! %s", SDL_GetError());
    }
}

bool safe_strncpy(char* destination, const char* source, size_t num) {
    /*
     * Safely copy a string from source to destination.
     *
     * Copies at most `num` bytes * after the first null character of `source` are not copied.
     * If no null character is encountered within the first `num` bytes
     * of `source`, `destination[num - 1]` will be manually set to zero,
     * so `destination` is guaranteed to be null terminated, unless
     * `num` is zero, in which case the `destination` is left unchanged.
     *
     * Arguments:
     *     destination: Address to copy to. Should have at least `num` bytes available.
     *     source: Address to copy from.
     *     num: Number of bytes to copy.
     *
     * Return:
     *     True if all bytes of source were copied (i.e. strlen(source) <= num - 1)
     */
    if (num > 0) {
        size_t i;
        for (i = 0; i < num - 1 && source[i] != '\0'; i++) {
            destination[i] = source[i];
        }
        destination[i] = '\0';
        return source[i] == '\0';
    }
    return false;
}

#include "fractal.v"
char* fractal_git_revision() { return FRACTAL_GIT_REVISION; }
