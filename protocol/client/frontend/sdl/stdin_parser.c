#include <whist/logging/logging.h>
#include <whist/core/whist.h>
#include <whist/core/whist_memory.h>
#include <whist/core/whist_string.h>
#include <whist/core/platform.h>
#include "stdin_parser.h"

#define INCOMING_MAXLEN (MAX_URL_LENGTH * MAX_NEW_TAB_URLS)

int get_next_piped_argument(char **key, char **value) {
    FATAL_ASSERT(key != NULL);
    FATAL_ASSERT(value != NULL);

#if OS_IS(OS_WIN32)
    DWORD bytes_available;
    if (PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &bytes_available, NULL) == 0) {
        LOG_ERROR("Error in PeekNamedPipe: %d", GetLastError());
        return -1;
    }
#else
    int bytes_available;
    if (ioctl(STDIN_FILENO, FIONREAD, &bytes_available) < 0) {
        LOG_ERROR("Error in ioctl: (%d) %s", errno, strerror(errno));
        return -1;
    }
#endif

    if (bytes_available == 0) {
        return 1;
    }

    char *incoming = safe_malloc(INCOMING_MAXLEN + 1);

    if (fgets(incoming, INCOMING_MAXLEN + 1, stdin) == NULL) {
        free(incoming);
        return -1;
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
