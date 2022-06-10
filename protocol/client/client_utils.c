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

bool using_stun = false;

MouseMotionAccumulation mouse_state = {0};

extern unsigned short port_mappings[USHRT_MAX + 1];

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

    WhistStatus ret = whist_parse_command_line(argc, (const char **)argv, &parse_operand);
    if (ret != WHIST_SUCCESS) {
        printf("Failed to parse command line!\n");
        return -1;
    }

    return 0;
}

int client_handle_dynamic_args(WhistFrontend *frontend) {
    while (true) {
        WhistFrontendEvent event;
        // TODO: Technically, file_drop events at this time cause memory leaks,
        // as we ignore those events and don't free the memory. We ignore these
        // for now because such events should not occur before actual startup.
        // A potential robust solution would be to integrate this handler with
        // handle_frontend_events() in handle_frontend_events.c.
        bool got_event = whist_frontend_wait_event(frontend, &event, 100);
        if (!got_event) {
            continue;
        }

        if (event.type == FRONTEND_EVENT_STARTUP_PARAMETER) {
            FrontendStartupParameterEvent *startup_parameter = &event.startup_parameter;
            char *key = startup_parameter->key;
            char *value = startup_parameter->value;

            if (startup_parameter->error) {
                // key/value are NULL
                return -1;
            }

            if (!strcmp(key, "finished")) {
                free(key);
                free(value);
                return 0;
            } else if (!strcmp(key, "kill")) {
                free(key);
                free(value);
                return 1;
            }

            WhistStatus opt_ret = whist_set_single_option(key, value);
            switch (opt_ret) {
                case WHIST_SUCCESS: {
                    LOG_INFO("Successfully set option '%s'", key);
                    break;
                }
                case WHIST_ERROR_NOT_FOUND: {
                    LOG_WARNING("Dynamic argument '%s' not available", key);
                    break;
                }
                default: {
                    LOG_WARNING("Failed to set option '%s': %s", key, whist_error_string(opt_ret));
                    break;
                }
            }
            free(key);
            free(value);
        }
    }
}

bool client_args_valid(void) {
    const char *server_ip = NULL;
    if (whist_option_get_string_value("server-ip", &server_ip) != WHIST_SUCCESS ||
        server_ip == NULL) {
        LOG_ERROR(
            "Need IP: if not passed in directly, IP must "
            "be passed in using the dynamic argument `server-ip`");
        return false;
    }
    return true;
}

int update_mouse_motion(WhistFrontend *frontend) {
    /*
        Update mouse location if the mouse state has updated since the last call
        to this function.

        Return:
            (int): 0 on success, -1 on failure
    */

    if (mouse_state.update) {
        int x, y, x_nonrel, y_nonrel; // , virtual_width, virtual_height;

        // Calculate x location of mouse cursor
        x_nonrel = mouse_state.x_nonrel * MOUSE_SCALING_FACTOR / output_width;
        if (x_nonrel < 0) {
            x_nonrel = 0;
        } else if (x_nonrel >= MOUSE_SCALING_FACTOR) {
            x_nonrel = MOUSE_SCALING_FACTOR - 1;
        }

        // Calculate y location of mouse cursor
        y_nonrel = mouse_state.y_nonrel * MOUSE_SCALING_FACTOR / output_height;
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
    whist_frontend_get_window_pixel_size(frontend, 0, &wcmsg.dimensions.width,
                                         &wcmsg.dimensions.height);
    wcmsg.dimensions.dpi = whist_frontend_get_window_dpi(frontend);

    LOG_INFO("Sending MESSAGE_DIMENSIONS: output=%dx%d, DPI=%d", wcmsg.dimensions.width,
             wcmsg.dimensions.height, wcmsg.dimensions.dpi);
    send_wcmsg(&wcmsg);
    udp_handle_resize(&packet_udp_context, wcmsg.dimensions.dpi);
}
