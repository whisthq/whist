#ifndef SERVER_PARSE_ARGS_H
#define SERVER_PARSE_ARGS_H
/**
 * Copyright Fractal Computers, Inc. 2021
 * @file server_parse_args.h
 * @brief This file contains all code that parses the arguments
 *        for the call to the server.
============================
Usage
============================

server_parse_args() parses the arguments passed to the call to
the server.
*/

/*
============================
Public Functions
============================
*/

/**
 * @brief                          server_parse_args() parses the arguments passed
 *                                 to the call to the server.
 *
 * @param argc                     Length of the argv array
 *
 * @param argv                     Array of arguments
 *
 * @returns                        Returns 0 on success, -1 on invalid args,
 *                                 and 1 on help or version args.
 */
int server_parse_args(int argc, char* argv[]);

#endif  // SERVER_PARSE_ARGS_H
