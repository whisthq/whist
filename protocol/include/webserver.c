/*
 * This file contains the implementation of the functions to query the Fractal
 * webserver to login and logout the user.

 Protocol version: 1.0
 Last modification: 11/28/2019

 By: Philippe Noël

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
#endif

#ifdef __cplusplus
extern "C" {
#endif

// send JSON post to query the database, authenticate the user and return the VM IP
char *sendJSONPost(char *path, char *jsonObj) {
  // initialize Winsock if this is a Windows client
  #if defined(_WIN32)
    WSADATA wsa;
    // initialize Winsock (sockets library)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
      printf("Failed. Error Code : %d.\n", WSAGetLastError());
      return (char *) WSAGetLastError();
    }
    printf("Winsock Initialised.\n");
  #endif





  // Open socket
  SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct hostent *host;
  host = gethostbyname("cube-celery-vm.herokuapp.com");

  printf("%s", host);

  struct sockaddr_in SocketAddress;
  SocketAddress.sin_family = AF_INET;
  SocketAddress.sin_port = htons(80); // HTTP port
  SocketAddress.sin_addr.s_addr = ((unsigned long) host->h_addr);

  if (connect(Socket,(SOCKADDR*)(&SocketAddress),sizeof(SocketAddress)) < 0) {
      printf("ERROR connecting %d\n", WSAGetLastError());
      return 0;
  }

  char message[5000];
  sprintf(message, "POST %s HTTP/1.0\r\nHost: cube-celery-vm.herokuapp.com\r\nContent-Type: application/json\r\nContent-Length:%d\r\n\r\n%s", path, strlen(jsonObj), jsonObj);

  send(Socket, message, strlen(message), 0);
  char buffer[4096];
  int len;
  len = recv(Socket,buffer,4096,0);
  int i,z;
  for(i=0;!(buffer[i]=='\r' && buffer[i+1]=='\n' && buffer[i+2]=='\r' && buffer[i+3]=='\n');i++);
  i+=4;
  char* buffer2;
  buffer2 = calloc(sizeof(char),len);
  for(z=0;i<len;i++)
      buffer2[z++] = buffer[i];
  closesocket(Socket);
  WSACleanup();
  return buffer2;





}

// log the user in and log its connection time
char *login(char *username, char *password) {
  // var to store the usr credentials
	char *credentials;

  // generate JSON logout format
  char  *jsonFrame = "{\"username\" : \"%s\", \"password\" : \"%s\"}";
  size_t jsonSize  = strlen(jsonFrame) + strlen(username) + strlen(password) - 1;
  char  *jsonObj   = malloc(jsonSize);

  // send the logout JSON to log the user in
  if (jsonObj != NULL) {
    // write JSON, callback since we start the app
    snprintf(jsonObj, jsonSize, jsonFrame, username, password);
    char *path = "/user/login";

    // send JSON to authenticate and free memory
    credentials = sendJSONPost(path, jsonObj);
    free(jsonObj);
    return credentials;
  }
  else {
    return ""; // error message
  }
}

// log the logout time of a user
int32_t logout(char *username) {
  // generate JSON logout format
  char  *jsonFrame = "{\"username\" : \"%s\"}";
  size_t jsonSize  = strlen(jsonFrame) + strlen(username) - 1;
  char  *jsonObj   = malloc(jsonSize);

  // send the logout JSON to log the logout time
  if (jsonObj != NULL) {
    // write JSON, no callback since we terminate the app
    snprintf(jsonObj, jsonSize, jsonFrame, username);
    char *path = "/tracker/logoff";

    // send JSON and free memory
    sendJSONPost(path, jsonObj);
    free(jsonObj);
  }
  return 0;
}

#ifdef __cplusplus
}
#endif

// renable Windows warning, if Windows client
#if defined(_WIN32)
  #pragma warning(default: 4996)
#endif
