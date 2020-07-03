#ifndef DESKTOP_UTILS_H
#define DESKTOP_UTILS_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audio.h
 * @brief This file contains all code that interacts directly with processing
 *        audio packets on the client.
============================
Usage
============================

initAudio() must be called first before receiving any audio packets.
updateAudio() gets called immediately after to update the client to the server's
audio format.
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
