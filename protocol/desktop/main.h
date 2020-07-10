#ifndef DESKTOP_MAIN_H
#define DESKTOP_MAIN_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.h
 * @brief This file contains helper functions used for the client to send
 *        Fractal messages to the server.
============================
Usage
============================

Use SendFmsg to send a FractalClientMessage to the server.
*/

/*
============================
Includes
============================
*/

#include "../fractal/core/fractal.h"

/*
============================
Defines
============================
*/

#define HOST_PUBLIC_KEY                                           \
    "ecdsa-sha2-nistp256 "                                        \
    "AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBOT1KV+" \
    "I511l9JilY9vqkp+QHsRve0ZwtGCBarDHRgRtrEARMR6sAPKrqGJzW/"     \
    "Zt86r9dOzEcfrhxa+MnVQhNE8="

#define HOST_PUBLIC_KEY_PATH "ssh_host_ecdsa_key.pub"
#define CLIENT_PRIVATE_KEY_PATH "sshkey"
#define MAX_NUM_CONNECTION_ATTEMPTS (3)

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Send a FractalMessage from client to server
 *
 * @param fmsg                     FractalMessage struct to send as packet
 *
 * @returns                        0 if succeeded, else -1
 */
int SendFmsg(FractalClientMessage* fmsg);

#endif  // DESKTOP_MAIN_H
