/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file client_utils.c
 * @brief This file contains helper functions for WhistClient
============================
Usage
============================

Call these functions from anywhere within client where they're
needed.
*/

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
#include <whist/logging/logging.h>
#include <whist/logging/error_monitor.h>
#include <whist/debug/debug_console.h>
#include "whist/utils/command_line.h"

/*
============================
Bad Globals [TODO: Remove these or give them `static`!]
============================
*/

// Taken from main.c
volatile char client_binary_aes_private_key[16];
volatile char client_hex_aes_private_key[33];
extern int output_width;
extern int output_height;
extern int override_bitrate;
extern SocketContext packet_udp_context;

// From main.c
volatile bool update_bitrate = false;

bool using_stun = false;

MouseMotionAccumulation mouse_state = {0};

extern unsigned short port_mappings[USHRT_MAX + 1];

static bool using_piped_arguments;

#define INCOMING_MAXLEN 2048 * 1000  //  MAX_URL_LENGTH * MAX_NEW_TAB_URLS
/*
============================
Command-line options
============================
*/

static WhistStatus set_private_key(const WhistCommandLineOption *opt, const char *value) {
    if (!read_hexadecimal_private_key((char *)value, (char *)client_binary_aes_private_key,
                                      (char *)client_hex_aes_private_key)) {
        printf("Invalid hexadecimal string: %s\n", value);
        return WHIST_ERROR_SYNTAX;
    }
    return WHIST_SUCCESS;
}

static WhistStatus set_port_mapping(const WhistCommandLineOption *opt, const char *value) {
    char separator = '.';
    char c = separator;
    unsigned short origin_port;
    unsigned short destination_port;
    const char *str = value;
    while (c == separator) {
        int bytes_read;
        int args_read =
            sscanf(str, "%hu:%hu%c%n", &origin_port, &destination_port, &c, &bytes_read);
        // If we read port arguments, then map them
        if (args_read >= 2) {
            LOG_INFO("Mapping port: origin=%hu, destination=%hu", origin_port, destination_port);
            port_mappings[origin_port] = destination_port;
        } else {
            char invalid_s[13];
            unsigned short invalid_s_len = (unsigned short)min(bytes_read + 1, 13);
            safe_strncpy(invalid_s, str, invalid_s_len);
            printf("Unable to parse the port mapping \"%s\"", invalid_s);
            return WHIST_ERROR_SYNTAX;
        }
        // if %c was the end of the string, exit
        if (args_read < 3) {
            break;
        }
        // Progress the string forwards
        str += bytes_read;
    }
    return WHIST_SUCCESS;
}

COMMAND_LINE_INT_OPTION(output_width, 'w', "width", MIN_SCREEN_WIDTH, MAX_SCREEN_WIDTH,
                        "Set the width for the windowed-mode window, "
                        "if both width and height are specified.")
COMMAND_LINE_INT_OPTION(output_height, 'h', "height", MIN_SCREEN_HEIGHT, MAX_SCREEN_HEIGHT,
                        "Set the height for the windowed-mode window, "
                        "if both width and height are specified.")
COMMAND_LINE_INT_OPTION(override_bitrate, 'o', "override-bitrate", 1, INT_MAX,
                        "Override the video bitrate with the provided value")

COMMAND_LINE_CALLBACK_OPTION(set_private_key, 'k', "private-key", WHIST_OPTION_REQUIRED_ARGUMENT,
                             "Pass in the RSA Private Key as a hexadecimal string.")
COMMAND_LINE_CALLBACK_OPTION(set_port_mapping, 'p', "ports", WHIST_OPTION_REQUIRED_ARGUMENT,
                             "Pass in custom port:port mappings, period-separated.  "
                             "Default: identity mapping.")

COMMAND_LINE_BOOL_OPTION(using_piped_arguments, 'r', "read-pipe",
                         "Read arguments from stdin until EOF.  Don't need to pass "
                         "in IP if using this argument and passing with arg `ip`.")

static WhistStatus parse_operand(int pos, const char *value) {
    // The first operand maps to the server-ip option.  Any subsequent
    // operands are an error.
    static bool ip_set = false;
    if (!ip_set) {
        ip_set = true;
        return whist_set_single_option("server-ip", value);
    } else {
        return WHIST_ERROR_ALREADY_SET;
    }
}

/*
============================
Public Function Implementations
============================
*/

int client_parse_args(int argc, const char *argv[]) {
    // Initialize private key to default
    memcpy((char *)&client_binary_aes_private_key, DEFAULT_BINARY_PRIVATE_KEY,
           sizeof(client_binary_aes_private_key));
    memcpy((char *)&client_hex_aes_private_key, DEFAULT_HEX_PRIVATE_KEY,
           sizeof(client_hex_aes_private_key));

    // default user email
    whist_set_single_option("user", "None");

    using_piped_arguments = false;

    WhistStatus ret = whist_parse_command_line(argc, (const char **)argv, &parse_operand);
    if (ret != WHIST_SUCCESS) {
        printf("Failed to parse command line!\n");
        return -1;
    }

    return 0;
}

int read_piped_arguments(bool run_only_once) {
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
        return -2;
    }
#endif
    // Each argument will be passed via pipe from the client application
    //    with the argument name and value separated by a "?"
    //    and each argument/value pair on its own line
    do {
        if (!run_only_once) {
            // to keep the fan from freaking out
            // If stdin doesn't have any characters, continue the loop
            // TODO: Block/poll with a 50 ms timeout on the stdin fd instead
            whist_sleep(50);
        }

#ifndef _WIN32
        if (ioctl(STDIN_FILENO, FIONREAD, &available_chars) < 0) {
            LOG_ERROR("ioctl error with piped arguments: %s", strerror(errno));
            return -2;
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

            int opt_ret = whist_set_single_option(arg_name, arg_value);
            if (opt_ret == WHIST_SUCCESS) {
                // Success, it was a normal option.
            } else if (opt_ret != WHIST_ERROR_NOT_FOUND) {
                // The option existed but parsing failed.
                LOG_WARNING("Error settng piped arg %s: %s.", arg_name,
                            whist_error_string(opt_ret));
            } else if (strlen(arg_name) == 2 && !strncmp(arg_name, "ip", strlen(arg_name))) {
                // If arg_name is `ip`, then set IP address
                if (!arg_value) {
                    LOG_WARNING("Must pass arg_value with `ip` arg_name");
                } else {
                    whist_set_single_option("server-ip", arg_value);
                    LOG_INFO("Connecting to IP %s", arg_value);
                }
            } else if (strlen(arg_name) == 4 && !strncmp(arg_name, "kill", strlen(arg_name))) {
                // If arg_name is `kill`, then return indication for graceful exit
                LOG_INFO("Killing client app");
                return 1;
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
    } while (keep_reading && !run_only_once);

    const char *server_ip = NULL;
    if (whist_option_get_string_value("server-ip", &server_ip) != WHIST_SUCCESS ||
        server_ip == NULL) {
        LOG_ERROR(
            "Need IP: if not passed in directly, IP must "
            "be passed in via pipe with arg name `ip`");
        return -1;
    }

    return 0;
}

int update_mouse_motion(WhistFrontend *frontend) {
    /*
        Update mouse location if the mouse state has updated since the last call
        to this function.

        Return:
            (int): 0 on success, -1 on failure
    */

    if (mouse_state.update) {
        int x, y, x_nonrel, y_nonrel, virtual_width, virtual_height;
        whist_frontend_get_window_virtual_size(frontend, &virtual_width, &virtual_height);

        // Calculate x location of mouse cursor
        x_nonrel = mouse_state.x_nonrel * MOUSE_SCALING_FACTOR / virtual_width;
        if (x_nonrel < 0) {
            x_nonrel = 0;
        } else if (x_nonrel >= MOUSE_SCALING_FACTOR) {
            x_nonrel = MOUSE_SCALING_FACTOR - 1;
        }

        // Calculate y location of mouse cursor
        y_nonrel = mouse_state.y_nonrel * MOUSE_SCALING_FACTOR / virtual_height;
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
        WhistClientMessage wcmsg = {0};
        wcmsg.type = MESSAGE_MOUSE_MOTION;
        wcmsg.mouseMotion.relative = mouse_state.is_relative;
        wcmsg.mouseMotion.x = x;
        wcmsg.mouseMotion.y = y;
        wcmsg.mouseMotion.x_nonrel = x_nonrel;
        wcmsg.mouseMotion.y_nonrel = y_nonrel;
        if (send_wcmsg(&wcmsg) != 0) {
            return -1;
        }

        mouse_state.update = false;
        mouse_state.x_rel = 0;
        mouse_state.y_rel = 0;
    }
    return 0;
}

void send_message_dimensions(WhistFrontend *frontend) {
    // Let the server know the new dimensions so that it
    // can change native dimensions for monitor
    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_DIMENSIONS;
    wcmsg.dimensions.width = output_width;
    wcmsg.dimensions.height = output_height;
    wcmsg.dimensions.dpi = whist_frontend_get_window_dpi(frontend);

    LOG_INFO("Sending MESSAGE_DIMENSIONS: output=%dx%d, DPI=%d", wcmsg.dimensions.width,
             wcmsg.dimensions.height, wcmsg.dimensions.dpi);
    send_wcmsg(&wcmsg);
    udp_handle_resize(&packet_udp_context, wcmsg.dimensions.dpi);
}
