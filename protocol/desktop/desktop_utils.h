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

typedef struct mouse_motion_accumulation {
    int x_rel;
    int y_rel;
    int x_nonrel;
    int y_nonrel;
    bool is_relative;
    bool update;
} mouse_motion_accumulation;

/*
============================
Includes
============================
*/

int parse_args(int argc, char* argv[]);

char* getLogDir(void);

int logConnectionID(int connection_id);

int initSocketLibrary(void);

int destroySocketLibrary(void);

int configureCache(void);

/**
 * Used to tell the server the user, which is used for sentry, along with other init information
 * @param email user email
 * @return 0 for success, -1 for failure
 */
int prepareInitToServer(FractalDiscoveryRequestMessage* fmsg, char* email);

int updateMouseMotion();

#endif  // DESKTOP_UTILS_H
