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

#define MAX_INIT_CONNECTION_ATTEMPTS (6)
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
 * @brief                          Parse the arguments passed into the client application
 *
 * @param argc                     Number of arguments
 *
 * @param argv                     List of arguments
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int client_parse_args(int argc, char* argv[]);

/**
 * @brief                          Read arguments from the stdin pipe if `using_piped_arguments`
 *                                 is set to `true`.
 *
 * @param keep_waiting             Pointer to a boolean indicating whether to continue waiting
 * @param run_only_once            A boolean controlling whether to keep polling for stdin updates
 * or read from stdin only one time. Must use 'true' if this function is being called in a while
 * loop.
 *
 * @returns                        Returns -1 on failure, 0 on success, 1 on args prompting graceful
 * exit
 */
int read_piped_arguments(bool* keep_waiting, bool run_only_once);

/**
 * @brief                          Init any allocated memory for parsed args
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int alloc_parsed_args(void);

/**
 * @brief                          Free any allocated memory for parsed args
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int free_parsed_args(void);

/**
 * @brief                          Update mouse location if the mouse state has
 *                                 updated since the last call to this function.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int update_mouse_motion(void);

/**
 * @brief                          Sends message to server with output dimensions and DPI.
 */
void send_message_dimensions(void);

/**
 * @brief                          Sends message to server with new tab URL to open,
 *                                 if the global new_tab_url is non-NULL.
 */
void send_new_tab_url_if_needed(void);

#endif  // CLIENT_UTILS_H
