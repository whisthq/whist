/*
 * General Whist helper functions and headers.
 *
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 **/

/*
============================
Includes
============================
*/

#include "whist.h"  // header file for this protocol, includes winsock
#include <whist/logging/logging.h>
#include <whist/fec/fec.h>
#include <whist/utils/atomic.h>

#include <ctype.h>
#include <stdio.h>

#include "features.h"
#include "../utils/sysinfo.h"

/*
============================
Private Function Implementations
============================
*/

static int multithreaded_print_system_info(void *opaque) {
    /*
        Thread function to print system info to log
    */

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

/*
============================
Public Function Implementations
============================
*/

void whist_init_subsystems(void) {
    whist_init_multithreading();
    whist_init_logger();
    whist_init_features();
    init_fec();
    whist_init_networking();
}

void print_system_info(void) {
    /*
        Print the system info of the computer
    */

    WhistThread sysinfo_thread =
        whist_create_thread(multithreaded_print_system_info, "print_system_info", NULL);
    whist_detach_thread(sysinfo_thread);
}

int runcmd(const char *cmdline, char **response) {
    /*
        Run a system command via command prompt or terminal

        Arguments:
            cmdline (const char*): String of the system command to run
            response (char**):  Pointer to a buffer that will be filled with the terminal output
                                from the cmdline on two conditions: the passed pointer is non-NULL
                                and reading the piped output from the command succeeds.

        Returns:
            (int): 0 or value of pipe if success, else -1
    */

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

    if (strlen((const char *)cmdline) + 1 > sizeof(cmd_buf)) {
        LOG_WARNING("runcmd cmdline too long!");
        if (response) {
            *response = NULL;
        }
        return -1;
    }

    memcpy(cmd_buf, cmdline, strlen((const char *)cmdline) + 1);

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

        DynamicBuffer *db = init_dynamic_buffer(false);
        for (;;) {
            b_success = ReadFile(h_child_std_out_rd, ch_buf, sizeof(ch_buf), &dw_read, NULL);
            if (!b_success || dw_read == 0) break;

            size_t original_size = db->size;
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
        int size = (int)db->size;
        // The caller will have to free this later
        *response = db->buf;
        // Free the DynamicBuffer struct
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
        FILE *p_pipe;

        /* Run DIR so that it writes its output to a pipe. Open this
         * pipe with read text attribute so that we can read it
         * like a text file.
         */

        char *cmd = safe_malloc(strlen(cmdline) + 128);
        snprintf(cmd, strlen(cmdline) + 128, "%s 2>/dev/null", cmdline);

        if ((p_pipe = popen(cmd, "r")) == NULL) {
            LOG_WARNING("Failed to popen %s", cmd);
            free(cmd);
            return -1;
        }
        free(cmd);

        /* Read pipe until end of file, or an error occurs. */

        int current_len = 0;
        DynamicBuffer *db = init_dynamic_buffer(false);

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

        // The caller will have to free this later
        *response = db->buf;

        // Free the DynamicBuffer struct
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

bool read_hexadecimal_private_key(const char *hex_string, char *binary_private_key,
                                  char *hex_private_key) {
    /*
        Reads a 16-byte hexadecimal string and copies it into private_key

        Arguments:
            hex_string (char*): The hexadecimal string to copy
            binary_private_key (char*): The 16-byte buffer to copy the bytes into
            hex_private_key (char*): The 33-byte buffer to fill with a hex string
                representation of the private key.

        Returns:
            (bool): true if hex_string was a 16-byte hexadecimal
                value, otherwise false
    */

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
             (unsigned int)htonl(*((uint32_t *)(binary_private_key))),
             (unsigned int)htonl(*((uint32_t *)(binary_private_key + 4))),
             (unsigned int)htonl(*((uint32_t *)(binary_private_key + 8))),
             (unsigned int)htonl(*((uint32_t *)(binary_private_key + 12))));

    return true;
}

int get_wcmsg_size(WhistClientMessage *wcmsg) {
    /*
        Calculate the size of a WhistClientMessage struct

        Arguments:
            wcmsg (WhistClientMessage*): The Whist Client Message to find the size

        Returns:
            (int): The size of the Whist Client Message struct
    */

    if (wcmsg->type == MESSAGE_KEYBOARD_STATE || wcmsg->type == CMESSAGE_INIT) {
        return sizeof(*wcmsg);
    } else if (wcmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*wcmsg) + wcmsg->clipboard.size;
    } else if (wcmsg->type == CMESSAGE_FILE_METADATA) {
        return (int)(sizeof(*wcmsg) + wcmsg->file_metadata.filename_len);
    } else if (wcmsg->type == CMESSAGE_FILE_DATA) {
        return (int)(sizeof(*wcmsg) + wcmsg->file.size);
    } else if (wcmsg->type == MESSAGE_OPEN_URL) {
        return (int)(sizeof(*wcmsg) + strlen((const char *)&wcmsg->urls_to_open) + 1);
    } else {
        // Send small wcmsg's when we don't need unnecessarily large ones
        // TODO : Remove this hardcoded value.
        return sizeof(wcmsg->type) + sizeof(wcmsg->id) + 40;
    }
}

void terminate_protocol(WhistExitCode exit_code) {
    /*
        Terminates the protocol
    */

    static atomic_int terminate_protocol_counter = ATOMIC_VAR_INIT(0);

    // This variable counts reentrancies
    // TODO: This currently counts simultaneous calls as well,
    // not a big deal but something thread_local's can fix
    int total_times_called = atomic_fetch_add(&terminate_protocol_counter, 1) + 1;

    if (total_times_called == 2) {
        // If this is the first reentrancy, try to just stacktrace
        printf("Error: terminate_protocol has been called twice\n");
        fflush(stdout);
        print_stacktrace();
        exit(WHIST_EXIT_FAILURE);
    } else if (total_times_called == 3) {
        // If we've reentered even more times, print_stacktrace must have failed too
        printf("Error: terminate_protocol has been called three times\n");
        fflush(stdout);
        exit(WHIST_EXIT_FAILURE);
    } else if (total_times_called > 3) {
        // We've reentered way too many times, just give up
        abort();
    }

    LOG_INFO("Terminating Protocol");

    if (exit_code != WHIST_EXIT_SUCCESS) {
        print_stacktrace();
    }

    destroy_logger();

    exit(exit_code);
}

bool safe_strncpy(char *destination, const char *source, size_t num) {
    /*
        Safely copy a string from source to destination.

        Copies at most `num` bytes * after the first null character of `source` are not copied.
        If no null character is encountered within the first `num` bytes
        of `source`, `destination[num - 1]` will be manually set to zero,
        so `destination` is guaranteed to be null terminated, unless
        `num` is zero, in which case the `destination` is left unchanged.

        Arguments:
            destination: Address to copy to. Should have at least `num` bytes available.
            source: Address to copy from.
            num: Number of bytes to copy.

        Return:
            (bool): true if all bytes of source were copied (i.e. strlen(source) <= num - 1),
                else false
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

void trim_utf8_string(char *str) {
    /*
     * This function takes a null-terminated string and modifies it to remove
     * any dangling unicode characters that may have been left over from a
     * previous truncation. This is necessary because dangling unicode characters
     * will cause horrible crashes if they are not removed.
     *
     * Arguments:
     *     str (char*): The utf8 string to be trimmed.
     */

    // On Windows, utf-8 seems to behave weirdly, causing major issues in the
    // below codepath. Therefore, I've disabled this function on Windows.
    // TODO: Fix this on Windows and re-enable the unit test.
#ifndef _WIN32
    // UTF-8 can use up to 4 bytes per character, so
    // in the malformed case, we would at most need to
    // trim 3 trailing bytes.

    size_t len = strlen(str);

    // Does a 4-byte sequence start at b[len - 3]?
    if (len >= 3 && (str[len - 3] & 0xf0) == 0xf0) {
        // Yes, so we need to trim 3 bytes.
        str[len - 3] = '\0';
        return;
    }

    // Does a 3 or more byte sequence start at str[len - 2]?
    if (len >= 2 && (str[len - 2] & 0xe0) == 0xe0) {
        // Yes, so we need to trim 2 bytes.
        str[len - 2] = '\0';
        return;
    }

    // Does a 2 or more byte sequence start at str[len - 1]?
    if (len >= 1 && (str[len - 1] & 0xc0) == 0xc0) {
        // Yes, so we need to trim 1 byte.
        str[len - 1] = '\0';
        return;
    }
#endif  // _WIN32
}

/* position of bit within character */
#define BIT_CHAR(bit) ((bit) / CHAR_BIT)

/* array index for character containing bit */
#define BIT_IN_CHAR(bit) (1 << (CHAR_BIT - 1 - ((bit) % CHAR_BIT)))

/* number of characters required to contain number of bits */
#define BITS_TO_CHARS(bits) ((((bits)-1) / CHAR_BIT) + 1)

BitArray *bit_array_create(const unsigned int bits) {
    BitArray *ba;

    if (bits == 0) {
        errno = EDOM;
        return NULL;
    }

    /* allocate structure */
    ba = (BitArray *)malloc(sizeof(BitArray));

    if (ba == NULL) {
        /* malloc failed */
        errno = ENOMEM;
    } else {
        ba->numBits = bits;

        /* allocate array */
        ba->array = (unsigned char *)malloc(sizeof(unsigned char) * BITS_TO_CHARS(bits));

        if (ba->array == NULL) {
            /* malloc failed */
            errno = ENOMEM;
            free(ba);
            ba = NULL;
        }
    }

    return ba;
}

void bit_array_free(BitArray *ba) {
    if (ba != NULL) {
        if (ba->array != NULL) {
            free(ba->array);
        }
        free(ba);
    }
}

void bit_array_set_all(const BitArray *const ba) {
    unsigned bits;
    unsigned char mask;

    if (ba == NULL) {
        return; /* nothing to set */
    }

    /* set bits in all bytes to 1 */
    memset((void *)(ba->array), UCHAR_MAX, BITS_TO_CHARS(ba->numBits));

    /* zero any spare bits so increment and decrement are consistent */
    bits = ba->numBits % CHAR_BIT;
    if (bits != 0) {
        mask = UCHAR_MAX << (CHAR_BIT - bits);
        ba->array[BIT_CHAR(ba->numBits - 1)] = mask;
    }
}

void bit_array_clear_all(const BitArray *const ba) {
    if (ba == NULL) {
        return; /* nothing to clear */
    }

    /* set bits in all bytes to 0 */
    memset((void *)(ba->array), 0, BITS_TO_CHARS(ba->numBits));
}

int bit_array_set_bit(const BitArray *const ba, const unsigned int bit) {
    if (ba == NULL) {
        return 0; /* no bit to set */
    }

    if (ba->numBits <= bit) {
        errno = ERANGE;
        return -1; /* bit out of range */
    }

    ba->array[BIT_CHAR(bit)] |= BIT_IN_CHAR(bit);
    return 0;
}

void *bit_array_get_bits(const BitArray *const ba) {
    if (NULL == ba) {
        return NULL;
    }

    return ((void *)(ba->array));
}

int bit_array_test_bit(const BitArray *const ba, const unsigned int bit) {
    return ((ba->array[BIT_CHAR(bit)] & BIT_IN_CHAR(bit)) != 0);
}

int int_div_roundup(int a, int b) { return (a + b - 1) / b; }

// Include WHIST_GIT_REVISION
#include <whist.v>
// Defines to stringize a macro
#define xstr(s) str(s)
#define str(s) #s
const char *whist_git_revision(void) {
    /*
        Returns git revision if found

        Returns:
            (char*): git revision as string, or "none" if no git revision found
    */

#ifdef WHIST_GIT_REVISION
    return xstr(WHIST_GIT_REVISION);
#else
    return "none";
#endif
}
