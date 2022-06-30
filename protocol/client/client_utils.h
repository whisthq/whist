/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file client_utils.h
 * @brief TODO
============================
Usage
============================

Call these functions from anywhere within client where they're
needed.
*/

#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

/*
============================
Includes
============================
*/

#include <whist/network/network.h>
#include <whist/network/ringbuffer.h>
#include <whist/core/whist.h>
#include "frontend/frontend.h"

#define MAX_RECONNECTION_ATTEMPTS (10)
#define IP_MAXLEN (31)

/*
============================
Custom Types
============================
*/

typedef struct MouseMotionAccumulation {
    int x_rel;
    int y_rel;
    int x_nonrel;
    int y_nonrel;
    bool is_relative;
    bool update;
} MouseMotionAccumulation;

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Parse the arguments passed into the Whist frontend
 *
 * @param argc                     Number of arguments
 *
 * @param argv                     List of arguments
 *
 * @returns                        Returns -1 on failure, 0 on success, 1 on args prompting
 *                                 graceful exit (help, version, etc.)
 */
int client_parse_args(int argc, const char* argv[]);

/**
 * @brief                          Process and handle dynamic arguments until receiving a signal
 *                                 to proceed.

 * @details                        The SDL frontend, for example, responds to dynamic
 *                                 arguments passed via `key\n` or `key?value\n` to `stdin`. The
 *                                 `kill` argument signals the client to exit gracefully, whereas
 *                                 the `finished` argument signals the client to proceed. Other
 *                                 dynamic arguments set the corresponding command-line options.
 *
 * @param frontend                Frontend from which to receive dynamic arguments
 *
 * @returns                        -1 on failure, 0 on "proceed", 1 on args prompting graceful exit
 */
int client_handle_dynamic_args(WhistFrontend* frontend);

/**
 * @brief                          Validate that the client's arguments set by `client_parse_args`
 *                                 and `handle_dynamic_args` are valid.
 *
 * @details                        This function verifies that a server IP address is indeed set,
 *                                 and may be extended in the future to provide additional checks.
 *
 * @returns                        Returns `true` if the arguments are valid, `false` otherwise.
 */
bool client_args_valid(void);

/**
 * @brief                          Update mouse location if the mouse state has
 *                                 updated since the last call to this function.
 *
 * @param frontend                 The frontend to target for the mouse update
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int update_mouse_motion(WhistFrontend* frontend);

/**
 * @brief                          Sends message to server with output dimensions and DPI.
 *
 * @param frontend                 The frontend to target for the message
 */
void send_message_dimensions(WhistFrontend* frontend);

#endif  // CLIENT_UTILS_H
