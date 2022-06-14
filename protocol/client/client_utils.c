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
#include <whist/logging/logging.h>
#include <whist/core/whist_string.h>
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
extern int override_bitrate;
extern SocketContext packet_udp_context;

// From main.c
volatile bool update_bitrate = false;

bool using_stun = false;

MouseMotionAccumulation mouse_state = {0};

extern unsigned short port_mappings[USHRT_MAX + 1];

static bool using_piped_arguments;

#define INCOMING_MAXLEN (MAX_URL_LENGTH * MAX_NEW_TAB_URLS)
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

    using_piped_arguments = false;

    WhistStatus ret = whist_parse_command_line(argc, (const char **)argv, &parse_operand);
    if (ret != WHIST_SUCCESS) {
        printf("Failed to parse command line!\n");
        return -1;
    }

    return 0;
}

static int handle_piped_argument(const char *key, const char *value) {
    LOG_INFO("Piped argument: %s=%s", key, value);
    int opt_ret = whist_set_single_option(key, value);
    if (opt_ret == WHIST_SUCCESS) {
        // Success, it was a normal option.
    } else if (opt_ret != WHIST_ERROR_NOT_FOUND) {
        // The option existed but parsing failed.
        LOG_WARNING("Error setting piped arg %s: %s.", key, whist_error_string(opt_ret));
    } else if (strlen(key) == 2 && !strncmp(key, "ip", strlen(key))) {
        // If key is `ip`, then set IP address
        if (!value) {
            LOG_WARNING("Must pass value with `ip` key");
        } else {
            whist_set_single_option("server-ip", value);
            LOG_INFO("Connecting to IP %s", value);
        }
    } else if (strlen(key) == 4 && !strncmp(key, "kill", strlen(key))) {
        // If key is `kill`, then return indication for graceful exit
        LOG_INFO("Killing client app");
        return 2;
    } else if (strlen(key) == 8 && !strncmp(key, "finished", strlen(key))) {
        // If key is `finished`, then stop reading args from pipe
        LOG_INFO("Finished piping arguments");
        return 1;
    } else if (strlen(key) == 7 && !strncmp(key, "loading", strlen(key))) {
        LOG_INFO("Loading message found %s", value);
    } else {
        // If key is invalid, then log a warning, but continue
        LOG_WARNING("Piped arg %s not available", key);
    }
    return 0;
}

/**
 * Read stdin until a line of the form:
 * key?value\n or key\n is read.
 */
static int read_next_piped_argument(char **key, char **value) {
    FATAL_ASSERT(key != NULL);
    FATAL_ASSERT(value != NULL);

    char *incoming = safe_malloc(INCOMING_MAXLEN + 1);

    if (fgets(incoming, INCOMING_MAXLEN + 1, stdin) == NULL) {
        free(incoming);
        return -1;
    }

    trim_newlines(incoming);
    if (strlen(incoming) == 0) {
        free(incoming);
        return 1;
    }

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

int read_piped_arguments(bool run_only_once) {
    if (!using_piped_arguments) {
        return 0;
    }

    do {
        char *key;
        char *value;
        int ret = read_next_piped_argument(&key, &value);
        if (ret == -1) {
            return -2;
        } else if (ret == 1) {
            continue;
        }

        ret = handle_piped_argument(key, value);
        free(key);
        free(value);
        if (ret == 2) {
            return 1;
        } else if (ret == 1) {
            return 0;
        }
    } while (!run_only_once);

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
    whist_frontend_get_window_pixel_size(frontend, &wcmsg.dimensions.width,
                                         &wcmsg.dimensions.height);
    wcmsg.dimensions.dpi = whist_frontend_get_window_dpi(frontend);

    LOG_INFO("Sending MESSAGE_DIMENSIONS: output=%dx%d, DPI=%d", wcmsg.dimensions.width,
             wcmsg.dimensions.height, wcmsg.dimensions.dpi);
    send_wcmsg(&wcmsg);
    udp_handle_resize(&packet_udp_context, wcmsg.dimensions.dpi);
}
