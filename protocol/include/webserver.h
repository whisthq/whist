/*
 * This file contains the headers of the functions to query the Fractal
 * webserver to login and logout the user.

 Protocol version: 1.0
 Last modification: 11/28/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#ifndef WEBSERVER_H_  // include guard
#define WEBSERVER_H_

// @brief sends a JSON POST request to the Fractal webservers
// @details authenticate the user and return the credentials
char *sendJSONPost(char *path, char *jsonObj);

// @brief logs the user in
// @details returns the credentials for the user's cloud computer
char *login(char *username, char *password);

// @brief logs out the user
// @details logs the time at which the user logged out
int32_t logout(char *username);

#endif // WEBSERVER_H_
