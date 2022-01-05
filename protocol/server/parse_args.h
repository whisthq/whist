#ifndef SERVER_PARSE_ARGS_H
#define SERVER_PARSE_ARGS_H
/**
 * Copyright (c) 2020-2022 Whist Technologies, Inc.
 * @file parse_args.h
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

typedef struct _whist_server_config whist_server_config;

/**
 * @brief                          server_parse_args() parses the arguments passed
 *                                 on the command line and fill the server_config struct
 *
 * @param config				   Server configuration
 *
 * @param argc                     Length of the argv array
 *
 * @param argv                     Array of arguments
 *
 * @returns                        Returns 0 on success, -1 on invalid args,
 *                                 and 1 on help or version args.
 */
int server_parse_args(whist_server_config* config, int argc, char* argv[]);

#endif  // SERVER_PARSE_ARGS_H
