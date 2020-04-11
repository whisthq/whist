/*
 * This file contains the implementation of the functions to query the Fractal
 * webserver to login and logout the user.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël, Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "webserver.h" // header file for this implementation file

// Windows libraries
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4996) // snprintf unsafe warning
#pragma warning(disable: 4047) // levels of indirection
#pragma warning(disable: 4311) // typecast warning
#pragma warning(disable: 4267) // size_t to int
#else
#define SOCKET int
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

// send JSON post to query the database, authenticate the user and return the VM IP
char* sendJSONPost( char* path, char* jsonObj )
{
    // environment variables
    SOCKET Socket; // socket to send/receive POST request
    struct hostent* host; // address struct of the host webserver
    struct sockaddr_in webserver_socketAddress; // address of the web server socket

    // Creating our TCP socket to connect to the web server
    Socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( Socket < 0 ) // Windows & Unix cases
    {
        // if can't create socket, return
        printf( "Could not create socket.\n" );
        return "2";
    }

    // get the host address of the web server
    host = gethostbyname( "cube-celery-vm.herokuapp.com" );

    // create the struct for the webserver address socket we will query
    webserver_socketAddress.sin_family = AF_INET;
    webserver_socketAddress.sin_port = htons( 80 ); // HTTP port
    webserver_socketAddress.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    // connect to the web server before sending the POST request packet
    int connect_status = connect( Socket, (struct sockaddr*) &webserver_socketAddress, sizeof( webserver_socketAddress ) );
    if( connect_status < 0 )
    {
        printf( "Could not connect to the webserver.\n" );
        return "3";
    }

    // now that we're connected, we can send the POST request to authenticate the user
    // first, we create the POST request message
    char message[5000];
    sprintf( message, "POST %s HTTP/1.0\r\nHost: cube-celery-vm.herokuapp.com\r\nContent-Type: application/json\r\nContent-Length:%zd\r\n\r\n%s", path, strlen( jsonObj ), jsonObj );

    // now we send it
    if( send( Socket, message, strlen( message ), 0 ) < 0 )
    {
        // error sending, terminate
        printf( "Sending POST message failed.\n" );
        return "4";
    }

    // now that it's sent, let's get the reply
    char buffer[4096]; // buffer to store the reply
    int len, i, z; // counters
    len = recv( Socket, buffer, 4096, 0 ); // get the reply

    // parse the reply
    for( i = 0; !(buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n'); i++ );
    i += 4;

    // get the parsed credentials
    char* credentials;
    credentials = calloc( sizeof( char ), len );
    for( z = 0; i < len; i++ )
    {
        credentials[z++] = buffer[i];
    }

    // Windows case, closing sockets
#if defined(_WIN32)
  // close the socket
    closesocket( Socket );
#else
    close( Socket );
#endif

    // return the user credentials if correct authentication, else empty
    return credentials;
}

// log the user in and log its connection time
char* login( char* username, char* password )
{
    // var to store the usr credentials
    char* credentials;

    // generate JSON logout format
    char* jsonFrame = "{\"username\" : \"%s\", \"password\" : \"%s\"}";
    size_t jsonSize = strlen( jsonFrame ) + strlen( username ) + strlen( password ) - 1;
    char* jsonObj = malloc( jsonSize );

    // send the logout JSON to log the user in
    if( jsonObj != NULL )
    {
        // write JSON, callback since we start the app
        snprintf( jsonObj, jsonSize, jsonFrame, username, password );
        char* path = "/user/login";

        // send JSON to authenticate and free memory
        credentials = sendJSONPost( path, jsonObj );
        free( jsonObj );
        return credentials;
    } else
    {
        return ""; // error message
    }
}

// log the logout time of a user
int32_t logout( char* username )
{
    // generate JSON logout format
    char* jsonFrame = "{\"username\" : \"%s\"}";
    size_t jsonSize = strlen( jsonFrame ) + strlen( username ) - 1;
    char* jsonObj = malloc( jsonSize );

    // send the logout JSON to log the logout time
    if( jsonObj != NULL )
    {
        // write JSON, no callback since we terminate the app
        snprintf( jsonObj, jsonSize, jsonFrame, username );
        char* path = "/tracker/logoff";

        // send JSON and free memory
        sendJSONPost( path, jsonObj );
        free( jsonObj );
    }
    return 0;
}

// parse the server response to get the VM IP address
char* parse_response( char* credentials )
{
    // output variable
    char* user_vm_ip = "";

    // vars used to parse
    char* trailing_string = "";
    char* leading_string;
    char* vm_key = "\"public_ip\":\"";
    bool found = false;

    // while we haven't walked the whole response
    while( *credentials )
    {
        // get trailing and leading strings
        size_t len = strlen( trailing_string );
        leading_string = malloc( len + 1 + 1 );
        strcpy( leading_string, trailing_string );

        // format leadning string to compare
        leading_string[len] = *credentials++;
        leading_string[len + 1] = '\0';

        // if we found it and it's not a slash
        if( found && strstr( leading_string, "\"" ) )
        {
            // found it, break
            user_vm_ip = trailing_string;
            break;
        }

        // increment leading string
        trailing_string = leading_string;
        free( leading_string );

        // reset trailing string
        if( strstr( leading_string, vm_key ) != NULL )
        {
            trailing_string = "";
            found = true;
        }
    }

    // return IP found
    return user_vm_ip;
}

// re-enable Windows warning, if Windows client
#if defined(_WIN32)
#pragma warning(default: 4996)
#pragma warning(default: 4047)
#pragma warning(default: 4311)
#pragma warning(default: 4267)
#endif