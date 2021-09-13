/**
 * Copyright Fractal Computers, Inc. 2020
 * @file client_utils.c
 * @brief This file contains helper functions for FractalClient
============================
Usage
============================

Call these functions from anywhere within client where they're
needed.
*/

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS  // stupid Windows warnings
#endif

/*
============================
Includes
============================
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_utils.h"
#include "network.h"
#include "native_window_utils.h"
#include <fractal/logging/logging.h>
#include <fractal/logging/error_monitor.h>
#include <fractal/core/fractalgetopt.h>

extern volatile char binary_aes_private_key[16];
extern volatile char hex_aes_private_key[33];
extern volatile char *server_ip;
extern volatile int output_width;
extern volatile int output_height;
extern volatile char *program_name;
extern volatile CodecType output_codec_type;
extern volatile SDL_Window *window;

extern volatile int max_bitrate;

// This variables should stay as arrays - we call sizeof() on them
extern char user_email[FRACTAL_ARGS_MAXLEN + 1];
extern char icon_png_filename[FRACTAL_ARGS_MAXLEN + 1];

extern bool skip_taskbar;

extern bool using_stun;

extern MouseMotionAccumulation mouse_state;
extern volatile SDL_Window *window;

extern unsigned short port_mappings[USHRT_MAX];

volatile bool using_piped_arguments;
const struct option cmd_options[] = {{"width", required_argument, NULL, 'w'},
                                     {"height", required_argument, NULL, 'h'},
                                     {"bitrate", required_argument, NULL, 'b'},
                                     {"codec", required_argument, NULL, 'c'},
                                     {"private-key", required_argument, NULL, 'k'},
                                     {"user", required_argument, NULL, 'u'},
                                     {"environment", required_argument, NULL, 'e'},
                                     {"icon", required_argument, NULL, 'i'},
                                     {"ports", required_argument, NULL, 'p'},
                                     {"name", required_argument, NULL, 'n'},
                                     {"read-pipe", no_argument, NULL, 'r'},
                                     {"loading", required_argument, NULL, 'l'},
                                     {"skip-taskbar", no_argument, NULL, 's'},
                                     // these are standard for POSIX programs
                                     {"help", no_argument, NULL, FRACTAL_GETOPT_HELP_CHAR},
                                     {"version", no_argument, NULL, FRACTAL_GETOPT_VERSION_CHAR},
                                     // end with NULL-termination
                                     {0, 0, 0, 0}};
const char *usage;

#define INCOMING_MAXLEN 127
// Syntax: "a" for no_argument, "a:" for required_argument, "a::" for optional_argument
#define OPTION_STRING "w:h:b:c:k:u:e:i:z:p:n:rl:s"

/*
============================
Private Function Implementations
============================
*/

char *dupstring(char *s1) {
    /*
        Generate a copy of a string

        Arguments:
            s1 (char*): String to be copied

        Return:
            (char*): Copy of string, as a new pointer
    */

    size_t len = strlen(s1);
    char *s2 = safe_malloc(len * sizeof *s2);
    char *ret = s2;
    for (; *s1; s1++, s2++) *s2 = *s1;
    *s2 = *s1;
    return ret;
}

int evaluate_arg(int eval_opt, char *eval_optarg) {
    /*
        Evaluate an option given the optcode and the argument

        Arguments:
            eval_opt (int): optcode
            eval_optarg (char*): argument (can be NULL) passed with opt

        Returns:
            (int): -1 on failure, 0 on success
    */

    long int ret;
    char *endptr;

    // reset errno so that previous timeouts and errors don't interfere with arg parsing
    errno = 0;

    switch (eval_opt) {
        case 'w': {  // width
            ret = strtol(eval_optarg, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
                printf("%s", usage);
                return -1;
            }
            output_width = (int)ret;
            break;
        }
        case 'h': {  // height
            ret = strtol(eval_optarg, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
                printf("%s", usage);
                return -1;
            }
            output_height = (int)ret;
            break;
        }
        case 'b': {  // bitrate
            ret = strtol(eval_optarg, &endptr, 10);
            if (errno != 0 || *endptr != '\0' || ret > INT_MAX || ret < 0) {
                printf("%s", usage);
                return -1;
            }
            max_bitrate = (int)ret;
            break;
        }
        case 'c': {  // codec
            if (!strcmp(eval_optarg, "h264")) {
                output_codec_type = CODEC_TYPE_H264;
            } else if (!strcmp(eval_optarg, "h265")) {
                output_codec_type = CODEC_TYPE_H265;
            } else {
                printf("Invalid codec type: '%s'\n", eval_optarg);
                printf("%s", usage);
                return -1;
            }
            break;
        }
        case 'k': {  // private key
            if (!read_hexadecimal_private_key(eval_optarg, (char *)binary_aes_private_key,
                                              (char *)hex_aes_private_key)) {
                printf("Invalid hexadecimal string: %s\n", eval_optarg);
                printf("%s", usage);
                return -1;
            }
            break;
        }
        case 'u': {  // user email
            if (!safe_strncpy(user_email, eval_optarg, sizeof(user_email))) {
                printf("User email is too long: %s\n", eval_optarg);
                return -1;
            }
            break;
        }
        case 'e': {  // environment
            error_monitor_set_environment(optarg);
            break;
        }
        case 'i': {  // protocol window icon
            if (!safe_strncpy(icon_png_filename, eval_optarg, sizeof(icon_png_filename))) {
                printf("Icon PNG filename is too long: %s\n", eval_optarg);
                return -1;
            }
            break;
        }
        case 'p': {  // port mappings
            char separator = '.';
            char c = separator;
            unsigned short origin_port;
            unsigned short destination_port;
            const char *str = eval_optarg;
            while (c == separator) {
                int bytes_read;
                int args_read =
                    sscanf(str, "%hu:%hu%c%n", &origin_port, &destination_port, &c, &bytes_read);
                // If we read port arguments, then map them
                if (args_read >= 2) {
                    LOG_INFO("Mapping port: origin=%hu, destination=%hu", origin_port,
                             destination_port);
                    port_mappings[origin_port] = destination_port;
                } else {
                    char invalid_s[13];
                    unsigned short invalid_s_len = (unsigned short)min(bytes_read + 1, 13);
                    safe_strncpy(invalid_s, str, invalid_s_len);
                    printf("Unable to parse the port mapping \"%s\"", invalid_s);
                    break;
                }
                // if %c was the end of the string, exit
                if (args_read < 3) {
                    break;
                }
                // Progress the string forwards
                str += bytes_read;
            }
            break;
        }
        case 'n': {  // window title
            program_name = calloc(sizeof(char), strlen(eval_optarg));
            strcpy((char *)program_name, eval_optarg);
            break;
        }
        case 'r': {  // use arguments piped from stdin
            using_piped_arguments = true;
            break;
        }
        case 'l': {  // loading message
            LOG_INFO("LOADING: %s", eval_optarg);
            break;
        }
        case 's': {  // skip taskbar
            skip_taskbar = true;
            break;
        }
        default: {
            if (eval_opt != -1) {
                // illegal option
                printf("%s", usage);
                return -1;
            }
            break;
        }
    }
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

int client_parse_args(int argc, char *argv[]) {
    printf("Entering client_parse_args with argc: %i, argv[0]: %s, argv[1]: %s \n", argc, argv[0], argv[1]);
    /*
        Parse the arguments passed into the client application

        Arguments:
            argc (int): number of arguments
            argv (char* []): array of arguments

        Return:
            (int): 0 on success and -1 on failure
    */

    // TODO: replace `client` with argv[0]
    usage =
        "Usage: client [OPTION]... IP_ADDRESS\n"
        "Try 'client --help' for more information.\n";
    const char *usage_details =
        "Usage: client [OPTION]... IP_ADDRESS\n"
        "\n"
        "All arguments to both long and short options are mandatory.\n"
        // regular options should look nice, with 2-space indenting for multiple lines
        "  -w, --width=WIDTH             Set the width for the windowed-mode\n"
        "                                  window, if both width and height\n"
        "                                  are specified\n"
        "  -h, --height=HEIGHT           Set the height for the windowed-mode\n"
        "                                  window, if both width and height\n"
        "                                  are specified\n"
        "  -b, --bitrate=BITRATE         Set the maximum bitrate to use\n"
        "  -c, --codec=CODEC             Launch the protocol using the codec\n"
        "                                  specified: h264 (default) or h265\n"
        "  -k, --private-key=PK          Pass in the RSA Private Key as a \n"
        "                                  hexadecimal string\n"
        "  -u, --user=EMAIL              Tell Fractal the user's email. Default: None \n"
        "  -e, --environment=ENV         The environment the protocol is running in,\n"
        "                                  e.g production, staging, development. Default: none\n"
        "  -i, --icon=PNG_FILE           Set the protocol window icon from a 64x64 pixel png file\n"
        "  -p, --ports=PORTS             Pass in custom port:port mappings, period-separated.\n"
        "                                  Default: identity mapping\n"
        "  -n, --name=NAME               Set the window title. Default: Fractal\n"
        "  -r, --read-pipe               Read arguments from stdin until EOF. Don't need to pass\n"
        "                                  in IP if using this argument and passing with arg `ip`\n"
        "  -l, --loading                 Custom loading screen message\n"
        "  -s, --skip-taskbar            Launch the protocol without displaying an icon\n"
        "                                  in the taskbar\n"
        // special options should be indented further to the left
        "      --help     Display this help and exit\n"
        "      --version  Output version information and exit\n";

    // Initialize private key to default
    memcpy((char *)&binary_aes_private_key, DEFAULT_BINARY_PRIVATE_KEY,
           sizeof(binary_aes_private_key));
    memcpy((char *)&hex_aes_private_key, DEFAULT_HEX_PRIVATE_KEY, sizeof(hex_aes_private_key));

    // default user email
    safe_strncpy(user_email, "None", sizeof(user_email));

    // default icon filename
    safe_strncpy(icon_png_filename, "", sizeof(icon_png_filename));

    int opt;
    bool ip_set = false;
    using_piped_arguments = false;

    while (true) {
        opt = getopt_long(argc, argv, OPTION_STRING, cmd_options, NULL);
        if (opt != -1 && optarg && strlen(optarg) > FRACTAL_ARGS_MAXLEN) {
            printf("Option passed into %c is too long! Length of %zd when max is %d\n", opt,
                   strlen(optarg), FRACTAL_ARGS_MAXLEN);
            return -1;
        }

        // For arguments that are not `help` and `version`, evaluate option
        //    and argument via `evaluate_arg`
        switch (opt) {
            case FRACTAL_GETOPT_HELP_CHAR: {  // help
                printf("%s", usage_details);
                printf("client_parse_args is returning 1 (help)!\n");
                return 1;
            }
            case FRACTAL_GETOPT_VERSION_CHAR: {  // version
                printf("Fractal client revision %s\n", fractal_git_revision());
                printf("client_parse_args is returning 1 (version)!\n");
                return 1;
            }
            default: {
                if (evaluate_arg(opt, optarg) < 0) {
                    printf("client_parse_args is returning -1 !\n");
                    return -1;
                }
            }
        }

        if (opt == -1) {
            if (optind < argc && !ip_set) {
                // there's a valid non-option arg and ip is unset
                safe_strncpy((char *)server_ip, argv[optind], IP_MAXLEN + 1);
                ip_set = true;
                ++optind;
            } else if (optind < argc || (!ip_set && !using_piped_arguments)) {
                // incorrect usage
                printf("%s", usage);
                printf("client_parse_args is returning -1 (incorrect usage)!\n");
                return -1;
            } else {
                // we're done
                break;
            }
        }
    }

    printf("client_parse_args is returning 0!\n");
    return 0;
}

int read_piped_arguments(bool *keep_waiting) {
    /*
        Read arguments from the stdin pipe if `using_piped_arguments` is
        set to `true`.

        Can read IP as argument with name `ip`

        Arguments:
            keep_waiting (bool*): pointer to a boolean indicating whether to continue waiting

        Returns:
            (int): 0 on success, -1 on failure
    */

    if (!using_piped_arguments) {
        return 0;
    }

    // Arguments will arrive from the client application via pipe to stdin
    char incoming[INCOMING_MAXLEN + 1];

    int total_stored_chars = 0;
    char read_char = 0;
    bool keep_reading = true;
    bool finished_line = false;

#ifndef _WIN32
    int available_chars;
#else
    DWORD available_chars;

    HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD stdin_filetype = GetFileType(h_stdin);
    if (stdin_filetype != FILE_TYPE_PIPE) {
        LOG_ERROR("Stdin must be piped in on Windows");
        return -1;
    }
#endif

    // Each argument will be passed via pipe from the client application
    //    with the argument name and value separated by a "?"
    //    and each argument/value pair on its own line
    while (keep_reading && *keep_waiting) {
        SDL_Delay(50);  // to keep the fan from freaking out
                        // If stdin doesn't have any characters, continue the loop
#ifndef _WIN32
        if (ioctl(STDIN_FILENO, FIONREAD, &available_chars) < 0) {
            LOG_ERROR("ioctl error with piped arguments: %s", strerror(errno));
            return -1;
        } else if (available_chars == 0) {
            continue;
        }
#else
        // When in piped mode (e.g. from the client app), stdin is a NamedPipe
        if (!PeekNamedPipe(h_stdin, NULL, 0, NULL, &available_chars, NULL)) {
            if (GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_PIPE_NOT_CONNECTED) {
                // On closed stdin, fgetc will return 0 for EOF, so force a char read to eval line
                available_chars = 1;
            }
        } else if (available_chars == 0) {
            continue;
        }
#endif  // _WIN32

        // Reset `incoming` so that it is at the very least initialized.
        memset(incoming, 0, INCOMING_MAXLEN + 1);

        for (int char_idx = 0; char_idx < (int)available_chars; char_idx++) {
            // Read a character from stdin
            read_char = (char)fgetc(stdin);

            // If the character is EOF, make sure the loop ends after this iteration
            if (read_char == EOF) {
                keep_reading = false;
            } else if (!finished_line) {
                incoming[total_stored_chars] = read_char;
                total_stored_chars++;
            }

            // Causes some funky behavior if the line being read in is longer than 128 characters
            // because
            //   it splits into two and processes as two different pieces
            if (!keep_reading ||
                (total_stored_chars > 0 && ((incoming[total_stored_chars - 1] == '\n') ||
                                            total_stored_chars == INCOMING_MAXLEN))) {
                finished_line = true;
                total_stored_chars = 0;
            } else {
                continue;
            }

            // We could use `strsep`, but it sadly is not cross-platform.
            // We split at the first occurence of '?'; the first part becomes
            // the name, and the last part becomes the value.
            // If there is no '?', the value is set to NULL.
            char *c;
            for (c = incoming; *c && *c != '?'; ++c)
                ;
            char *arg_name = incoming;
            char *arg_value = NULL;
            if (*c == '?') {
                *c = '\0';
                arg_value = c + 1;
            }

            if (!arg_name) {
                goto completed_line_eval;
            }

            if (arg_value) {
                arg_value[strcspn(arg_value, "\n")] = 0;  // removes trailing newline, if exists
                arg_value[strcspn(arg_value, "\r")] =
                    0;  // removes trailing carriage return, if exists
            }

            arg_name[strcspn(arg_name, "\n")] = 0;  // removes trailing newline, if exists
            arg_name[strcspn(arg_name, "\r")] = 0;  // removes trailing carriage return, if exists

            // Iterate through cmd_options to find the corresponding opt
            int opt_index = -1;
            for (int i = 0; cmd_options[i].name; i++) {
                if (strncmp(arg_name, cmd_options[i].name, strlen(arg_name))) continue;

                if (strlen(cmd_options[i].name) == (unsigned)strlen(arg_name)) {
                    opt_index = i;
                    break;
                }
            }

            if (opt_index >= 0) {
                // Evaluate the passed argument, if a valid opt
                if (evaluate_arg(cmd_options[opt_index].val, arg_value) < 0) {
                    LOG_ERROR("Piped arg %s with value %s wasn't accepted", arg_name,
                              arg_value ? arg_value : "NULL");
                    return -1;
                }
            } else if (strlen(arg_name) == 2 && !strncmp(arg_name, "ip", strlen(arg_name))) {
                // If arg_name is `ip`, then set IP address
                if (!arg_value) {
                    LOG_WARNING("Must pass arg_value with `ip` arg_name");
                } else {
                    safe_strncpy((char *)server_ip, arg_value, IP_MAXLEN + 1);
                    LOG_INFO("Connecting to IP %s", server_ip);
                }
            } else if (strlen(arg_name) == 4 && !strncmp(arg_name, "kill", strlen(arg_name))) {
                // If arg_name is `kill`, then return failure
                LOG_INFO("Killing client app");
                return 0;
            } else if (strlen(arg_name) == 8 && !strncmp(arg_name, "finished", strlen(arg_name))) {
                // If arg_name is `finished`, then stop reading args from pipe
                LOG_INFO("Finished piping arguments");
                keep_reading = false;
                goto end_of_eval_loop;
            } else if (strlen(arg_name) == 7 && !strncmp(arg_name, "loading", strlen(arg_name))) {
                LOG_INFO("Loading message found %s", arg_value);
            } else {
                // If arg_name is invalid, then log a warning, but continue
                LOG_WARNING("Piped arg %s not available", arg_name);
            }

            fflush(stdout);

        completed_line_eval:
            if (finished_line) {
                // Reset finished_line after evaluating a line
                finished_line = false;
                memset(incoming, 0, INCOMING_MAXLEN + 1);
            }
        }
    end_of_eval_loop:
        available_chars = 0;
    }

    if (*keep_waiting && strlen((char *)server_ip) == 0) {
        LOG_ERROR(
            "Need IP: if not passed in directly, IP must be passed in via pipe with arg name `ip`");
        return -1;
    }

    return 0;
}

int init_socket_library(void) {
    /*
        Initialize the Windows socket library
        (Does not do anything for non-Windows)

        Return:
            (int): 0 on success, -1 on failure
    */

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        LOG_INFO("Failed to initialize Winsock with error code: %d.", WSAGetLastError());
        return -1;
    }
#endif
    return 0;
}

int destroy_socket_library(void) {
    /*
        Destroy the Windows socket library
        (Does not do anything for non-Windows)

        Return:
            (int): 0 on success, -1 on failure
    */

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

int alloc_parsed_args(void) {
    /*
        Init any allocated memory for parsed args

        Return:
            (int): 0 on success, -1 on failure
    */
    server_ip = safe_malloc(IP_MAXLEN + 1);

    if (!server_ip) {
        return -1;
    }

    memset((char *)server_ip, 0, IP_MAXLEN + 1);

    return 0;
}

int free_parsed_args(void) {
    /*
        Free any allocated memory for parsed args

        Return:
            (int): 0 on success, -1 on failure
    */
    if (server_ip) {
        free((char *)server_ip);
    }

    return 0;
}

int prepare_init_to_server(FractalDiscoveryRequestMessage *fmsg, char *email) {
    /*
        Prepare for initial request to server by setting
        user email and time data

        Arguments:
            fmsg (FractalDiscoveryRequestMessage*): pointer to the discovery
                request message packet to be sent to the server
            email (char*): user email

        Return:
            (int): 0 on success, -1 on failure
    */

    // Copy email
    if (!safe_strncpy(fmsg->user_email, email, sizeof(fmsg->user_email))) {
        LOG_ERROR("User email is too long: %s.\n", email);
        return -1;
    }
    // Copy time
    if (get_time_data(&(fmsg->time_data)) != 0) {
        LOG_ERROR("Failed to get time data.");
        return -1;
    }

    // Let the server know what OS we are
#ifdef _WIN32
    fmsg->os = FRACTAL_WINDOWS;
#elif defined(__APPLE__)
    fmsg->os = FRACTAL_APPLE;
#else
    fmsg->os = FRACTAL_LINUX;
#endif

    return 0;
}

int update_mouse_motion() {
    /*
        Update mouse location if the mouse state has updated since the last call
        to this function.

        Return:
            (int): 0 on success, -1 on failure
    */

    if (mouse_state.update) {
        int window_width, window_height;
        SDL_GetWindowSize((SDL_Window *)window, &window_width, &window_height);
        int x, y, x_nonrel, y_nonrel;

        // Calculate x location of mouse cursor
        x_nonrel = mouse_state.x_nonrel * MOUSE_SCALING_FACTOR / window_width;
        if (x_nonrel < 0) {
            x_nonrel = 0;
        } else if (x_nonrel >= MOUSE_SCALING_FACTOR) {
            x_nonrel = MOUSE_SCALING_FACTOR - 1;
        }

        // Calculate y location of mouse cursor
        y_nonrel = mouse_state.y_nonrel * MOUSE_SCALING_FACTOR / window_height;
        if (y_nonrel < 0) {
            y_nonrel = 0;
        } else if (y_nonrel >= MOUSE_SCALING_FACTOR) {
            y_nonrel = MOUSE_SCALING_FACTOR - 1;
        }

        // Set x and y of mouse
        if (mouse_state.is_relative) {
            x = mouse_state.x_rel;
            y = mouse_state.y_rel;
        } else {
            x = x_nonrel;
            y = y_nonrel;
        }

        // Send new mouse locations to server
        FractalClientMessage fmsg = {0};
        fmsg.type = MESSAGE_MOUSE_MOTION;
        fmsg.mouseMotion.relative = mouse_state.is_relative;
        fmsg.mouseMotion.x = x;
        fmsg.mouseMotion.y = y;
        fmsg.mouseMotion.x_nonrel = x_nonrel;
        fmsg.mouseMotion.y_nonrel = y_nonrel;
        if (send_fmsg(&fmsg) != 0) {
            return -1;
        }

        mouse_state.update = false;
        mouse_state.x_rel = 0;
        mouse_state.y_rel = 0;
    }
    return 0;
}

void send_message_dimensions() {
    // Let the server know the new dimensions so that it
    // can change native dimensions for monitor
    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_DIMENSIONS;
    fmsg.dimensions.width = output_width;
    fmsg.dimensions.height = output_height;
    fmsg.dimensions.codec_type = output_codec_type;
    fmsg.dimensions.dpi = get_native_window_dpi((SDL_Window *)window);
    LOG_INFO("Sending MESSAGE_DIMENSIONS: output=%dx%d, DPI=%d, codec=%d", fmsg.dimensions.width,
             fmsg.dimensions.height, fmsg.dimensions.dpi, fmsg.dimensions.codec_type);
    send_fmsg(&fmsg);
}
