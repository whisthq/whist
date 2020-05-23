/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock

// Print Memory Info
#if defined(_WIN32)
#include <processthreadsapi.h>
#include <psapi.h>
#endif

void PrintMemoryInfo() {
#if defined(_WIN32)
    DWORD processID = GetCurrentProcessId();
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    // Print information about the memory usage of the process.

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                           processID);
    if (NULL == hProcess) return;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        LOG_INFO("PeakWorkingSetSize: %lld", (long long)pmc.PeakWorkingSetSize);
        LOG_INFO("WorkingSetSize: %lld", (long long)pmc.WorkingSetSize);
    }

    CloseHandle(hProcess);
#endif
}
// End Print Memory Info

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

typedef enum json_type { JSON_BOOL, JSON_INT, JSON_STRING } json_type_t;

typedef struct kv_pair {
    json_type_t type;
    char* key;
    union {
        char* str_value;
        int int_value;
        bool bool_value;
    };
} kv_pair_t;

typedef struct json {
    int size;
    kv_pair_t* pairs;
} json_t;

void skip_whitespace(char** str) {
    while (**str == ' ' || **str == '\t' || **str == '\r' || **str == '\n') {
        (*str)++;
    }
}

char next_char(char** str) {
    (*str)++;
    return **str;
}

char next_alphanumeric_char(char** str) {
    next_char(str);
    skip_whitespace(str);
    return **str;
}

char* read_escaped_string(char** str) {
    char c;
    if (**str != '\"') {
        return NULL;
    }
    char* original_string = *str + 1;
    int str_len = 0;
    while ((c = next_char(str)) != '\"') {
        //LOG_INFO("CHAR: %c", c);
        str_len++;
    }
    char* return_str = malloc(str_len + 1);
    memcpy(return_str, original_string, str_len);
    return_str[str_len] = '\0';

    return return_str;
}

bool parse_json(char* str, json_t* json) {
    skip_whitespace(&str);
    if (*str != '{') {
        return false;
    }
    char c;

    kv_pair_t kv_pairs[100];
    int num_kv_pairs = 0;

    while ((c = next_alphanumeric_char(&str)) != '\0') {
        //LOG_INFO("CHAR: %c", c);

        kv_pair_t* kv = &kv_pairs[num_kv_pairs];
        num_kv_pairs++;

        if (*str != '\"') {
            LOG_ERROR("JSON KEY did not start with \"! Had %c", *str);
            return false;
        }

        kv->key = read_escaped_string(&str);
        //LOG_INFO("KEY BEGIN READ: %s", kv->key);

        if (next_alphanumeric_char(&str) != ':') {
            LOG_ERROR("JSON KEY did have a following :! Had %c", *str);
            return false;
        }

        if (next_alphanumeric_char(&str) != '\"') {
            if (memcmp(str, "true", 4) == 0) {
                str += 3;
                kv->type = JSON_BOOL;
                kv->bool_value = true;
            } else if (memcmp(str, "false", 5) == 0) {
                str += 4;
                kv->type = JSON_BOOL;
                kv->bool_value = false;
            } else if ('0' < *str && *str < '9') {
                kv->type = JSON_INT;
                kv->int_value = atoi(str);
                while ('0' <= *str && *str <= '9') {
                    str++;
                }
                str--;
            } else {
                kv = NULL;
                LOG_ERROR("JSON VALUE did not start with \"! Had %c", *str);
                return false;
            }
        } else {
            kv->type = JSON_STRING;
            kv->str_value = read_escaped_string(&str);
            //LOG_INFO("VALUE BEGIN READ: %s", kv->str_value);
        }

        /*
        if (kv == NULL) {
            LOG_INFO("VALUE ERROR");
            return false;
        } else {
            switch (kv->type) {
                case JSON_BOOL:
                    LOG_INFO("VALUE BEGIN: %s",
                                kv->bool_value ? "true" : "false");
                    break;
                case JSON_INT:
                    LOG_INFO("VALUE BEGIN: %d", kv->int_value);
                    break;
                case JSON_STRING:
                    LOG_INFO("VALUE BEGIN: \"%s\"", kv->str_value);
                    break;
            }
        }
        */

        c = next_alphanumeric_char(&str);

        if (c != ',' && c != '}') {
            LOG_ERROR("JSON VALUE did have a following , or }! Had %c",
                        *str);
            return false;
        }

        if (c == '}') {
            break;
        }
    }

    json->size = num_kv_pairs;
    json->pairs = malloc(sizeof(kv_pair_t) * num_kv_pairs);
    memcpy(json->pairs, kv_pairs, sizeof(kv_pair_t) * num_kv_pairs);

    /*
    for (int i = 0; i < json->size; i++) {
        kv_pair_t* kv = &json->pairs[i];
        LOG_INFO("KEY BEGIN: %s\n", kv->key);
        switch (kv->type) {
            case JSON_BOOL:
                LOG_INFO("VALUE BEGIN: %s", kv->bool_value ? "true" : "false");
                break;
            case JSON_INT:
                LOG_INFO("VALUE BEGIN: %d", kv->int_value);
                break;
            case JSON_STRING:
                LOG_INFO("VALUE BEGIN: \"%s\"", kv->str_value);
                break;
        }
    }
    */

    return true;
}

kv_pair_t* get_kv(json_t json, char* key) {
    for (int i = 0; i < json.size; i++) {
        if (strcmp(json.pairs[i].key, key) == 0) {
            return &json.pairs[i];
        }
    }
    return NULL;
}

void free_json(json_t json) {
    for (int i = 0; i < json.size; i++) {
        free(json.pairs[i].key);
        if (json.pairs[i].type == JSON_STRING) {
            free(json.pairs[i].str_value);
        }
    }
    free(json.pairs);
}

char* copy(char* str) {
    int len = strlen(str) + 1;
    char* ret_str = malloc(len);
    memcpy(ret_str, str, len);
    return ret_str;
}

static char* branch = NULL;
static bool is_dev;
static bool already_obtained_vm_type = false;

bool is_dev_vm() {
    if (already_obtained_vm_type) {
        return is_dev;
    }

    char buf[4800];
    size_t len = sizeof(buf);

    LOG_INFO("GETTING JSON");

    if (!SendJSONGet("cube-celery-staging.herokuapp.com", "/vm/isDev", buf,
                     len)) {
        return true;
    }

    char* json_str = NULL;
    for (int i = 0; i < len - 4; i++) {
        if (memcmp(buf + i, "\r\n\r\n", 4) == 0) {
            json_str = buf + i + 4;
        }
    }

    json_t json;
    parse_json(json_str, &json);

    kv_pair_t* dev_value = get_kv(json, "dev");
    kv_pair_t* branch_value = get_kv(json, "branch");
    if (dev_value && branch_value) {
        if (dev_value->type != JSON_BOOL) {
            return false;
        }

        is_dev = dev_value->bool_value;
        branch = copy(branch_value->str_value);

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

int GetFmsgSize(struct FractalClientMessage* fmsg) {
    if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
        return sizeof(*fmsg);
    } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*fmsg) + fmsg->clipboard.size;
    } else {
        return sizeof(fmsg->type) + 40;
    }
}
