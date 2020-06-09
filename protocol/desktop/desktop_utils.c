#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

extern volatile char *server_ip;
extern volatile int output_width;
extern volatile int output_height;
extern volatile int max_bitrate;
extern volatile bool is_spectator;

int parseArgs(int argc, char* argv[]) {
    char* usage =
        "Usage: desktop [IP ADDRESS] [[OPTIONAL] WIDTH]"
        " [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]"
        " [[OPTIONAL] SPECTATE] \n";
    int num_required_args = 1;
    int num_optional_args = 4;
    if (argc - 1 < num_required_args ||
        argc - 1 > num_required_args + num_optional_args) {
        printf("%s", usage);
        return -1;
    }

    server_ip = argv[1];

    output_width = 0;
    output_height = 0;

    is_spectator = false;

    long int ret;
    char* endptr;

    if (argc >= 3) {
        errno = 0;
        ret = strtol(argv[2], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
            printf("%s", usage);
            return -1;
        }
        if (ret != 0) output_width = (int)ret;
    }

    if (argc >= 4) {
        errno = 0;
        ret = strtol(argv[3], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
            printf("%s", usage);
            return -1;
        }
        if (ret != 0) output_height = (int)ret;
    }

    if (argc == 5) {
        errno = 0;
        ret = strtol(argv[4], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret <= 0) {
            printf("%s", usage);
            return -1;
        }
        max_bitrate = (int)ret;
    }

    if (argc == 6) {
        is_spectator = true;
    }

    return 0;
}

#ifdef _WIN32
char *getLogDir(void) {
    char *log_dir = malloc(2 * sizeof *log_dir);
    if (log_dir == NULL) {
        return NULL;
    }
    if (sprintf(log_dir, "%s", ".") < 0) {
        free(log_dir);
        return NULL;
    }
    return log_dir;
}
#else
static char *appendPathToHome(char *path) {
    char *home, *new_path;
    int len;

    home = getenv("HOME");

    len = strlen(home) + strlen(path) + 2;
    new_path = malloc(len * sizeof *new_path);
    if (new_path == NULL) return NULL;

    if (sprintf(new_path, "%s/%s", home, path) < 0) {
        free(new_path);
        return NULL;
    }

    return new_path;
}

char *getLogDir(void) {
    return appendPathToHome(".fractal");
}
#endif

#ifdef _WIN32
int initSocketLibrary(void) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n",
                WSAGetLastError());
        return -1;
    }
    return 0;
}
int destroySocketLibrary(void) {
    WSACleanup();
    return 0;
}
#else
int initSocketLibrary(void) { return 0; };
int destroySocketLibrary(void) { return 0; };
#endif

/*
   Write ecdsa key to a local file for ssh to use, for that server ip
   This will identify the connecting server as the correct server and not an
   imposter
*/
static int writePublicSSHKey(void) {
    FILE* ssh_key_host = fopen(HOST_PUBLIC_KEY_PATH, "w");
    if (ssh_key_host == NULL) {
        return -1;
    }
    if (fprintf(ssh_key_host, "%s %s\n", server_ip, HOST_PUBLIC_KEY) < 0) {
        fclose(ssh_key_host);
        return -1;
    }
    fclose(ssh_key_host);
    return 0;
}

int configureSSHKeys(void) {
    if (writePublicSSHKey() != 0) {
        LOG_ERROR("Failed to write public ssh key to disk.");
        return -1;
    }

    #ifndef _WIN32
    if (chmod(CLIENT_PRIVATE_KEY_PATH, 600) != 0) {
        LOG_ERROR("Failed to make host's private ssh (at %s) readable and"
            "writable. (Error: %s)", CLIENT_PRIVATE_KEY_PATH, strerror(errno));
        return -1;
    }
    #endif
    return 0;
}


// files can't be written to a macos app bundle, so they need to be
// cached in /Users/USERNAME/.APPNAME, here .fractal directory
// attempt to create fractal cache directory, it will fail if it
// already exists, which is fine
// for Linux, this is in /home/USERNAME/.fractal, the cache is also needed
// for the same reason
// the mkdir command won't do anything if the folder already exists, in
// which case we make sure to clear the previous logs and connection id

#ifdef _WIN32
int configureCache(void) { return 0; }
#else
int configureCache(void) {
    runcmd("mkdir ~/.fractal", NULL);
    runcmd("chmod 0755 ~/.fractal", NULL);
    runcmd("rm -f ~/.fractal/log.txt", NULL);
    runcmd("rm -f ~/.fractal/connection_id.txt", NULL);
    return 0;
}
#endif
