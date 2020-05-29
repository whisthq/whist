#ifndef DESKTOP_MAIN_H
#define DESKTOP_MAIN_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.h
 * @brief This file contains helper functions used for the client to send Fractal messages to the server.
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include "../fractal/core/fractal.h"

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
