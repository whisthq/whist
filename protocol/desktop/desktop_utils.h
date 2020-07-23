#ifndef DESKTOP_UTILS_H
#define DESKTOP_UTILS_H
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

int parseArgs(int argc, char* argv[]);

char* getLogDir(void);

int logConnectionID(int connection_id);

int initSocketLibrary(void);

int destroySocketLibrary(void);

int configureCache(void);

int configureSSHKeys(void);

int sendTimeToServer(void);

int updateMouseMotion();

#endif  // DESKTOP_UTILS_H
