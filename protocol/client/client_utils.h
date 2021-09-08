/**
 * Copyright Fractal Computers, Inc. 2020
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

#include <fractal/network/network.h>

#define MAX_INIT_CONNECTION_ATTEMPTS (3)
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
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int read_piped_arguments(bool* keep_waiting);

/**
 * @brief                          Initialize the Windows socket library
 *                                 (Does not do anything for non-Windows)
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int init_socket_library(void);

/**
 * @brief                          Destroy the Windows socket library
 *                                 (Does not do anything for non-Windows)
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int destroy_socket_library(void);

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
 * @brief                          Prepare for initial request to server by setting
 *                                 user email and time data
 *
 * @param fmsg                     Discovery request message packet to be sent to the server
 *
 * @param email                    User email
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int prepare_init_to_server(FractalDiscoveryRequestMessage* fmsg, char* email);

/**
 * @brief                          Update mouse location if the mouse state has
 *                                 updated since the last call to this function.
 *
 * @returns                        Returns -1 on failure, 0 on success
 */
int update_mouse_motion();

/**
 * @brief                          Sends message to server with output dimensions and DPI.
 */
void send_message_dimensions();

#endif  // CLIENT_UTILS_H
