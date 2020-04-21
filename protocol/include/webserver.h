/*
 * Helper functions for webserver querying via POST requests in C.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef WEBSERVER_H
#define WEBSERVER_H

bool sendLog();

// @brief sends a JSON POST request to the Fractal webservers
// @details authenticate the user and return the credentials
bool sendJSONPost( char* host_s, char* path, char* jsonObj );

char* parse_response( char* credentials );

#endif // WEBSERVER_H
