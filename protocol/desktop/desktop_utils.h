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

#endif  // DESKTOP_UTILS_H
