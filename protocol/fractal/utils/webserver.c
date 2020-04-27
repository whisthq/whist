/*
 * Helper functions for webserver querying via POST requests in C.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // silence the deprecated warnings
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "webserver.h"  // header file for this implementation file

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Windows libraries
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#define SOCKET int
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// send JSON post to query the database, authenticate the user and return the VM
// IP
bool sendJSONPost(char* host_s, char* path, char* jsonObj) {
    // environment variables
    SOCKET Socket;         // socket to send/receive POST request
    struct hostent* host;  // address struct of the host webserver
    struct sockaddr_in
        webserver_socketAddress;  // address of the web server socket

    // Creating our TCP socket to connect to the web server
    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket < 0)  // Windows & Unix cases
    {
        // if can't create socket, return
        printf("Could not create socket.\n");
        return false;
    }
    set_timeout(Socket, 250);

    // get the host address of the web server
    host = gethostbyname(host_s);

    // create the struct for the webserver address socket we will query
    webserver_socketAddress.sin_family = AF_INET;
    webserver_socketAddress.sin_port = htons(80);  // HTTP port
    webserver_socketAddress.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    // connect to the web server before sending the POST request packet
    int connect_status =
        connect(Socket, (struct sockaddr*)&webserver_socketAddress,
                sizeof(webserver_socketAddress));
    if (connect_status < 0) {
        printf("Could not connect to the webserver.\n");
        return false;
    }

    // now that we're connected, we can send the POST request to authenticate
    // the user first, we create the POST request message

    int json_len = strlen(jsonObj);
    char* message = malloc(5000 + json_len);
    int result = sprintf(message,
                         "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: "
                         "application/json\r\nContent-Length:%zd\r\n\r\n%s\0",
                         path, host_s, json_len, jsonObj);

    // now we send it
    if (send(Socket, message, (int)strlen(message), 0) < 0) {
        // error sending, terminate
        printf("Sending POST message failed.\n");
        return false;
    }

    free(message);

    // now that it's sent, let's get the reply
    char buffer[4096];                              // buffer to store the reply
    int len;                                        // counters
    len = recv(Socket, buffer, sizeof(buffer), 0);  // get the reply

    // get the parsed credentials
    for (int i = 0; i < len; i++) {
        if (buffer[i] == '\r') {
            buffer[i] = '\0';
        }
    }
    mprintf("Webserver Response: %s\n", buffer);

    // Windows case, closing sockets
#if defined(_WIN32)
    // close the socket
    closesocket(Socket);
#else
    close(Socket);
#endif
    // return the user credentials if correct authentication, else empty
    return true;
}

extern int connection_id;

bool sendLog() {
    char* host = "fractal-mail-staging.herokuapp.com";
    char* path = "/logs";

    char* logs_raw = get_mprintf_history();
    int raw_log_len = get_mprintf_history_len();

    char* logs = malloc(1000 + 2 * raw_log_len);
    int log_len = 0;
    for (int i = 0; i < raw_log_len; i++) {
        switch (logs_raw[i]) {
            case '\b':
                logs[log_len++] = '\\';
                logs[log_len++] = 'b';
                break;
            case '\f':
                logs[log_len++] = '\\';
                logs[log_len++] = 'f';
                break;
            case '\n':
                logs[log_len++] = '\\';
                logs[log_len++] = 'n';
                break;
            case '\r':
                logs[log_len++] = '\\';
                logs[log_len++] = 'r';
                break;
            case '\t':
                logs[log_len++] = '\\';
                logs[log_len++] = 't';
                break;
            case '"':
                logs[log_len++] = '\\';
                logs[log_len++] = '"';
                break;
            case '\\':
                logs[log_len++] = '\\';
                logs[log_len++] = '\\';
                break;
            default:
                logs[log_len++] = logs_raw[i];
                break;
        }
    }

    logs[log_len++] = '\0';

    char* json = malloc(1000 + log_len);
    sprintf(json,
            "{\
            \"connection_id\" : \"%d\",\
            \"logs\" : \"%s\",\
            \"sender\" : \"server\"\
    }\0",
            connection_id, logs);
    sendJSONPost(host, path, json);
    free(logs);
    free(json);

    return true;
}
