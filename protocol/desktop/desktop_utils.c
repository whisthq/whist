/**
 * Copyright Fractal Computers, Inc. 2020
 * @file desktop_utils.c
 * @brief TODO
============================
Usage
============================

TODO
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings
#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "desktop_utils.h"
#include "network.h"
#include "../fractal/utils/logging.h"
#include "fractalgetopt.h"

extern volatile char aes_private_key[16];
extern volatile char *server_ip;
extern volatile int output_width;
extern volatile int output_height;
extern volatile char *program_name;
extern volatile CodecType output_codec_type;

extern volatile int max_bitrate;
extern volatile int running_ci;
extern char user_email[USER_EMAIL_MAXLEN];  // Note: Is larger than environment maxlen
extern char sentry_environment[FRACTAL_ENVIRONMENT_MAXLEN + 1];

extern volatile CodecType codec_type;
extern bool using_stun;

extern mouse_motion_accumulation mouse_state;
extern volatile SDL_Window *window;

extern unsigned short port_mappings[USHRT_MAX];

// standard for POSIX programs
#define FRACTAL_GETOPT_HELP_CHAR (CHAR_MIN - 2)
#define FRACTAL_GETOPT_VERSION_CHAR (CHAR_MIN - 3)

const struct option cmd_options[] = {{"width", required_argument, NULL, 'w'},
                                     {"height", required_argument, NULL, 'h'},
                                     {"bitrate", required_argument, NULL, 'b'},
                                     {"codec", required_argument, NULL, 'c'},
                                     {"private-key", required_argument, NULL, 'k'},
                                     {"user", required_argument, NULL, 'u'},
                                     {"environment", required_argument, NULL, 'e'},
                                     {"connection-method", required_argument, NULL, 'z'},
                                     {"ports", required_argument, NULL, 'p'},
                                     {"use_ci", no_argument, NULL, 'x'},
                                     {"name", required_argument, NULL, 'n'},
                                     // these are standard for POSIX programs
                                     {"help", no_argument, NULL, FRACTAL_GETOPT_HELP_CHAR},
                                     {"version", no_argument, NULL, FRACTAL_GETOPT_VERSION_CHAR},
                                     // end with NULL-termination
                                     {0, 0, 0, 0}};

#define OPTION_STRING "w:h:b:c:p:ku:e:z:n:"

int parseArgs(int argc, char *argv[]) {
    // todo: replace `desktop` with argv[0]
    const char *usage =
        "Usage: desktop [OPTION]... IP_ADDRESS\n"
        "Try 'desktop --help' for more information.\n";
    const char *usage_details =
        "Usage: desktop [OPTION]... IP_ADDRESS\n"
        "\n"
        "All arguments to both long and short options are mandatory.\n"
        "  -w, --width=WIDTH             set the width for the windowed-mode\n"
        "                                  window, if both width and height\n"
        "                                  are specified\n"
        "  -h, --height=HEIGHT           set the height for the windowed-mode\n"
        "                                  window, if both width and height\n"
        "                                  are specified\n"
        "  -b, --bitrate=BITRATE         set the maximum bitrate to use\n"
        "  -c, --codec=CODEC             launch the protocol using the codec\n"
        "                                  specified: h264 (default) or h265\n"
        "  -k, --private-key=PK          pass in the RSA Private Key as a "
        "                                  hexadecimal string\n"
        "  -u, --user=EMAIL              Tell fractal the users email. Optional defaults to None"
        "  -e, --environment=ENV         The environment the protocol is running \n"
        "                                 in. e.g master, staging, dev. Optional defaults to dev"
        "  -p, --ports=PORTS             Pass in custom port:port mappings, comma-separated\n"
        "  -x, --use_ci                  launch the protocol in CI mode\n"
        "  -z, --connection_method=CM    which connection method to try first,\n"
        "                                  either STUN or DIRECT\n"
        "  -n, --name=NAME               Set the window title (default: Fractal)\n"
        "      --help     display this help and exit\n"
        "      --version  output version information and exit\n";

    memcpy((char *)&aes_private_key, DEFAULT_PRIVATE_KEY, sizeof(aes_private_key));
    // default sentry environment
    strcpy(sentry_environment, "dev");
    // default sentry environment
    strcpy(user_email, "None");

    int opt;
    long int ret;
    bool ip_set = false;
    char *endptr;

    for (unsigned int i = 0; i < USHRT_MAX; i++) {
        port_mappings[i] = i;
    }

    while (true) {
        opt = getopt_long(argc, argv, OPTION_STRING, cmd_options, NULL);
        if (opt != -1 && optarg && strlen(optarg) > FRACTAL_ENVIRONMENT_MAXLEN) {
            printf("Option passed into %c is too long! Length of %zd when max is %d", opt,
                   strlen(optarg), FRACTAL_ENVIRONMENT_MAXLEN);
            return -1;
        }
        errno = 0;
        switch (opt) {
            case 'w':
                ret = strtol(optarg, &endptr, 10);
                if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
                    printf("%s", usage);
                    return -1;
                }
                output_width = (int)ret;
                break;
            case 'h':
                ret = strtol(optarg, &endptr, 10);
                if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
                    printf("%s", usage);
                    return -1;
                }
                output_height = (int)ret;
                break;
            case 'b':
                ret = strtol(optarg, &endptr, 10);
                if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
                    printf("%s", usage);
                    return -1;
                }
                max_bitrate = (int)ret;
                break;
            case 'c':
                if (!strcmp(optarg, "h264")) {
                    output_codec_type = CODEC_TYPE_H264;
                } else if (!strcmp(optarg, "h265")) {
                    output_codec_type = CODEC_TYPE_H265;
                } else {
                    printf("Invalid codec type: '%s'\n", optarg);
                    printf("%s", usage);
                    return -1;
                }
                break;
            case 'k':
                if (!read_hexadecimal_private_key(optarg, (char *)aes_private_key)) {
                    printf("Invalid hexadecimal string: %s\n", optarg);
                    printf("%s", usage);
                    return -1;
                }
                break;
            case 'u':
                strcpy(user_email, optarg);
                break;
            case 'e':
                strcpy(sentry_environment, optarg);
                break;
            case 'p': {
                char c = ',';
                unsigned short origin_port;
                unsigned short destination_port;
                const char *str = optarg;
                while (c == ',') {
                    int bytes_read;
                    int args_read = sscanf(str, "%hu:%hu%c%n", &origin_port, &destination_port, &c,
                                           &bytes_read);
                    // If we read port arguments, then map them
                    if (args_read >= 2) {
                        LOG_INFO("Mapping port: origin=%hu, destination=%hu", origin_port,
                                 destination_port);
                        port_mappings[origin_port] = destination_port;
                    } else {
                        char invalid_s[13];
                        unsigned short invalid_s_len = (unsigned short)min(bytes_read, 12);
                        strncpy(invalid_s, str, invalid_s_len);
                        invalid_s[invalid_s_len] = '\0';
                        LOG_WARNING("Unable to parse the parse mapping \"%s\"", invalid_s);
                    }
                    // if %c was the end of the string, exit
                    if (args_read < 3) {
                        break;
                    }
                    // Progress the string forwards
                    str += bytes_read;
                }
            } break;
            case 'x':
                running_ci = 1;
                break;
            case 'z':
                if (!strcmp(optarg, "STUN")) {
                    using_stun = true;
                } else if (!strcmp(optarg, "DIRECT")) {
                    using_stun = false;
                } else {
                    printf("Invalid connection type: '%s'\n", optarg);
                    printf("%s", usage);
                    return -1;
                }
                break;
            case 'n':
                program_name = calloc(sizeof(char), strlen(optarg));
                strcpy((char *)program_name, optarg);
                break;
            case FRACTAL_GETOPT_HELP_CHAR:
                printf("%s", usage_details);
                return 1;
            case FRACTAL_GETOPT_VERSION_CHAR:
                printf("Fractal client revision %s\n", FRACTAL_GIT_REVISION);
                return 1;
            default:
                if (opt != -1) {
                    // illegal option
                    printf("%s", usage);
                    return -1;
                }
                break;
        }
        if (opt == -1) {
            if (optind < argc && !ip_set) {
                // there's a valid non-option arg and ip is unset
                server_ip = argv[optind];
                ip_set = true;
                ++optind;
            } else if (optind < argc || !ip_set) {
                // incorrect usage
                printf("%s", usage);
                return -1;
            } else {
                // we're done
                break;
            }
        }
    }

    return 0;
}

#ifndef _WIN32
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
#endif

char *dupstring(char *s1) {
    size_t len = strlen(s1);
    char *s2 = malloc(len * sizeof *s2);
    char *ret = s2;
    if (s2 == NULL) return NULL;
    for (; *s1; s1++, s2++) *s2 = *s1;
    *s2 = *s1;
    return ret;
}

char *getLogDir(void) {
#ifdef _WIN32
    return dupstring(".");
#else
    return appendPathToHome(".fractal");
#endif
}

int logConnectionID(int connection_id) {
    // itoa is not portable
    char *str_connection_id = malloc(sizeof(char) * 100);
    sprintf(str_connection_id, "%d", connection_id);
    // send connection id to sentry as a tag, server also does this
    sentry_set_tag("connection_id", str_connection_id);

    char *path;
#ifdef _WIN32
    path = dupstring("connection_id.txt");
#else
    path = appendPathToHome(".fractal/connection_id.txt");
#endif
    if (path == NULL) {
        LOG_ERROR("Failed to get connection log path.");
        return -1;
    }

    FILE *f = fopen(path, "w");
    free(path);
    if (f == NULL) {
        LOG_ERROR("Failed to open connection id log file.");
        return -1;
    }
    if (fprintf(f, "%d", connection_id) < 0) {
        LOG_ERROR("Failed to write connection id to log file.");
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

int initSocketLibrary(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }
#endif
    return 0;
}

int destroySocketLibrary(void) {
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

/*
   Write ecdsa key to a local file for ssh to use, for that server ip
   This will identify the connecting server as the correct server and not an
   imposter
*/
int configureSSHKeys(void) {
    FILE *ssh_key_host = fopen(HOST_PUBLIC_KEY_PATH, "w");
    if (ssh_key_host == NULL) {
        LOG_ERROR("Failed to open public ssh key file.");
        return -1;
    }
    if (fprintf(ssh_key_host, "%s %s\n", server_ip, HOST_PUBLIC_KEY) < 0) {
        fclose(ssh_key_host);
        LOG_ERROR("Failed to write public ssh key to file.");
        return -1;
    }
    fclose(ssh_key_host);

#ifndef _WIN32
    if (chmod(CLIENT_PRIVATE_KEY_PATH, 600) != 0) {
        LOG_ERROR(
            "Failed to make host's private ssh (at %s) readable and"
            "writable. (Error: %s)",
            CLIENT_PRIVATE_KEY_PATH, strerror(errno));
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
int configureCache(void) {
#ifndef _WIN32
    runcmd("mkdir -p ~/.fractal", NULL);
    runcmd("chmod 0755 ~/.fractal", NULL);
    runcmd("rm -f ~/.fractal/log.txt", NULL);
    runcmd("rm -f ~/.fractal/connection_id.txt", NULL);
#endif
    return 0;
}

int sendTimeToServer(void) {
    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_TIME;
    if (GetTimeData(&(fmsg.time_data)) != 0) {
        LOG_ERROR("Failed to get time data.");
        return -1;
    }
    SendFmsg(&fmsg);

    return 0;
}

int sendEmailToServer(char *email) {
    struct FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_USER_EMAIL;
    strcpy(fmsg.user_email, email);

    if (SendFmsg(&fmsg) != 0) {
        return -1;
    } else {
        return 0;
    }
}

int updateMouseMotion() {
    if (mouse_state.update) {
        int window_width, window_height;
        SDL_GetWindowSize((SDL_Window *)window, &window_width, &window_height);
        int x, y, x_nonrel, y_nonrel;

        x_nonrel = mouse_state.x_nonrel * MOUSE_SCALING_FACTOR / window_width;
        y_nonrel = mouse_state.y_nonrel * MOUSE_SCALING_FACTOR / window_height;

        if (mouse_state.is_relative) {
            x = mouse_state.x_rel;
            y = mouse_state.y_rel;
        } else {
            x = x_nonrel;
            y = y_nonrel;
        }

        FractalClientMessage fmsg = {0};
        fmsg.type = MESSAGE_MOUSE_MOTION;
        fmsg.mouseMotion.relative = mouse_state.is_relative;
        fmsg.mouseMotion.x = x;
        fmsg.mouseMotion.y = y;
        fmsg.mouseMotion.x_nonrel = x_nonrel;
        fmsg.mouseMotion.y_nonrel = y_nonrel;
        if (SendFmsg(&fmsg) != 0) {
            return -1;
        }

        mouse_state.update = false;
        mouse_state.x_rel = 0;
        mouse_state.y_rel = 0;
    }
    return 0;
}
