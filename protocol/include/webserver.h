/*
 * Helper functions for webserver querying via POST requests in C.
 *
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef WEBSERVER_H
#define WEBSERVER_H

char* sendJSONPost( char* path, char* jsonObj );

char* parse_response( char* credentials );

#endif // WEBSERVER_H
