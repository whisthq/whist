/**
 * Copyright Fractal Computers, Inc. 2021
 * @file parse_args.c
 * @brief This file contains the code that parses the args for
the server protocol.
============================
Usage
============================

server_parse_args() checks and parses the args for server/main.c
*/

/*
============================
Includes
============================
*/

#include <fractal/core/fractal.h>
#include <fractal/core/fractalgetopt.h>
#include <fractal/logging/error_monitor.h>
#include "parse_args.h"
#include "network.h"

// i: means --identifier MUST take an argument
#define OPTION_STRING "k:i:w:e:t:"

extern char binary_aes_private_key[16];
extern char hex_aes_private_key[33];
extern char identifier[FRACTAL_IDENTIFIER_MAXLEN + 1];
extern int begin_time_to_exit;

/*
============================
Custom Types
============================
*/

// required_argument means --identifier MUST take an argument
const struct option cmd_options[] = {{"private-key", required_argument, NULL, 'k'},
                                     {"identifier", required_argument, NULL, 'i'},
                                     {"environment", required_argument, NULL, 'e'},
                                     {"timeout", required_argument, NULL, 't'},
                                     // these are standard for POSIX programs
                                     {"help", no_argument, NULL, FRACTAL_GETOPT_HELP_CHAR},
                                     {"version", no_argument, NULL, FRACTAL_GETOPT_VERSION_CHAR},
                                     // end with NULL-termination
                                     {0, 0, 0, 0}};

/*
============================
Public Function Implementations
============================
*/

int server_parse_args(int argc, char* argv[]) {
    // TODO: replace `server` with argv[0]
    const char* usage =
        "Usage: server [OPTION]... IP_ADDRESS\n"
        "Try 'server --help' for more information.\n";
    const char* usage_details =
        "Usage: server [OPTION]... IP_ADDRESS\n"
        "\n"
        "All arguments to both long and short options are mandatory.\n"
        // regular options should look nice, with 2-space indenting for multiple lines
        "  -k, --private-key=PK          Pass in the RSA Private Key as a\n"
        "                                  hexadecimal string. Defaults to\n"
        "                                  binary and hex default keys in\n"
        "                                  the protocol code\n"
        "  -i, --identifier=ID           Pass in the unique identifier for this\n"
        "                                  server as a hexadecimal string\n"
        "  -e, --environment=ENV         The environment the protocol is running in,\n"
        "                                  e.g production, staging, development. Default: none\n"
        "  -t, --timeout=TIME            Tell the server to give up after TIME seconds. If TIME\n"
        "                                  is -1, disable auto exit completely. Default: 60\n"
        // special options should be indented further to the left
        "      --help     Display this help and exit\n"
        "      --version  Output version information and exit\n";
    memcpy((char*)&binary_aes_private_key, DEFAULT_BINARY_PRIVATE_KEY,
           sizeof(binary_aes_private_key));
    memcpy((char*)&hex_aes_private_key, DEFAULT_HEX_PRIVATE_KEY, sizeof(hex_aes_private_key));

    int opt;

    while (true) {
        opt = getopt_long(argc, argv, OPTION_STRING, cmd_options, NULL);
        if (opt != -1 && optarg && strlen(optarg) > FRACTAL_ARGS_MAXLEN) {
            printf("Option passed into %c is too long! Length of %zd when max is %d\n", opt,
                   strlen(optarg), FRACTAL_ARGS_MAXLEN);
            return -1;
        }
        errno = 0;
        switch (opt) {
            case 'k': {
                if (!read_hexadecimal_private_key(optarg, (char*)binary_aes_private_key,
                                                  (char*)hex_aes_private_key)) {
                    printf("Invalid hexadecimal string: %s\n", optarg);
                    printf("%s", usage);
                    return -1;
                }
                break;
            }
            case 'i': {
                printf("Identifier passed in: %s\n", optarg);
                if (!safe_strncpy(identifier, optarg, sizeof(identifier))) {
                    printf("Identifier passed in is too long! Has length %lu but max is %d.\n",
                           (unsigned long)strlen(optarg), FRACTAL_IDENTIFIER_MAXLEN);
                    return -1;
                }
                break;
            }
            case 'e': {
                error_monitor_set_environment(optarg);
                break;
            }
            case 't': {
                printf("Timeout before autoexit passed in: %s\n", optarg);
                if (sscanf(optarg, "%d", &begin_time_to_exit) != 1 ||
                    (begin_time_to_exit <= 0 && begin_time_to_exit != -1)) {
                    printf("Timeout should be a positive integer or -1\n");
                }
                break;
            }
            case FRACTAL_GETOPT_HELP_CHAR: {
                printf("%s", usage_details);
                return 1;
            }
            case FRACTAL_GETOPT_VERSION_CHAR: {
                printf("Fractal client revision %s\n", fractal_git_revision());
                return 1;
            }
            default: {
                if (opt != -1) {
                    // illegal option
                    printf("%s", usage);
                    return -1;
                }
                break;
            }
        }
        if (opt == -1) {
            bool can_accept_nonoption_args = false;
            if (optind < argc && can_accept_nonoption_args) {
                // there's a valid non-option arg
                // Do stuff with argv[optind]
                ++optind;
            } else if (optind < argc && !can_accept_nonoption_args) {
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
