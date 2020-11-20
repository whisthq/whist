#ifndef DESKTOP_UTILS_H
#define DESKTOP_UTILS_H

#include "../fractal/network/network.h"

#define MAX_INIT_CONNECTION_ATTEMPTS (3)
#define MAX_RECONNECTION_ATTEMPTS (10)

/**
 * Copyright Fractal Computers, Inc. 2020
 * @file desktop_utils.c
 * @brief TODO
============================
Usage
============================

TODO
*/

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
Includes
============================
*/

int parse_args(int argc, char* argv[]);

char* get_log_dir(void);

int log_connection_id(int connection_id);

int init_socket_library(void);

int destroy_socket_library(void);

int configure_cache(void);

/**
 * Used to tell the server the user, which is used for sentry, along with other init information
 * @param email user email
 * @return 0 for success, -1 for failure
 */
int prepare_init_to_server(FractalDiscoveryRequestMessage* fmsg, char* email);

int update_mouse_motion();

#endif  // DESKTOP_UTILS_H
