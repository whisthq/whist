#include <whist/logging/logging.h>
#include <whist/core/whist.h>
#include <whist/core/whist_memory.h>
#include <whist/utils/threads.h>
#include <whist/core/whist_string.h>
#include <whist/core/platform.h>
#include "stdin_parser.h"

#define INCOMING_MAXLEN (MAX_URL_LENGTH * MAX_NEW_TAB_URLS)

static int stdin_bytes_available(void) {
#if OS_IS(OS_WIN32)
    DWORD bytes_available;
    if (PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &bytes_available, NULL) == 0) {
        DWORD error = GetLastError();
        // If the error is ERROR_INVALID_FUNCTION, then we are not running this in an environment
        // where stdin can be treated as a pipe; no need to log anything.
        if (error != ERROR_INVALID_FUNCTION) {
            LOG_ERROR("Error in PeekNamedPipe: %d", error);
        }
        return -1;
    }
#else
    int bytes_available;
    if (ioctl(STDIN_FILENO, FIONREAD, &bytes_available) < 0) {
        LOG_ERROR("Error in ioctl: (%d) %s", errno, strerror(errno));
        return -1;
    }
#endif

    return (int)bytes_available;
}

int get_next_piped_argument(char **key, char **value) {
    FATAL_ASSERT(key != NULL);
    FATAL_ASSERT(value != NULL);

    int bytes = stdin_bytes_available();
    if (bytes == -1) {
        return -1;
    } else if (bytes == 0) {
        return 1;
    }

    char *incoming = safe_malloc(INCOMING_MAXLEN + 1);
    size_t i = 0;
    while (i < INCOMING_MAXLEN) {
        int b = safe_read(STDIN_FILENO, &incoming[i], 1);
        if (b == -1) {
            LOG_ERROR("Failed to read from stdin: %d", errno);
            free(incoming);
            return -1;
        }

        if (b == 0) {
            whist_sleep(100);
            continue;
        }

        if (incoming[i] == '\n') {
            incoming[i] = '\0';
            break;
        }

        ++i;
    }

    trim_newlines(incoming);

    char *raw_value = split_string_at(incoming, "?");
    *key = strdup(incoming);
    if (raw_value) {
        trim_newlines(raw_value);
        *value = strdup(raw_value);
    } else {
        *value = NULL;
    }

    free(incoming);
    return 0;
}
