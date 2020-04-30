/*
 * Helper functions for webserver querying via POST requests in C.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "../core/fractal.h"
#include "../utils/logging.h"
#include "network.h"

bool sendLog();

// @brief sends a JSON POST request to the Fractal webservers
// @details authenticate the user and return the credentials
bool sendJSONPost(char* host_s, char* path, char* jsonObj);

#endif  // WEBSERVER_H
