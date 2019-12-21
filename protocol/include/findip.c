/*
 * This file contains the implementation of a simple program to find the IPv4
 * address of the computer it is run on.
 *
 * Fractal Protocol version 1.1
 *
 * Last modified: 12/20/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <unistd.h>
  #include <netdb.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

#include "findip.h" // header file for this file

// finds the IPv4 of the host computer
char *get_host_ipv4() {
  // initialize the windows socket library if this is a windows client
  #if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
      printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
      return "1";
    }
  #endif

  // environment variables
  char hostbuffer[256];
  char *IPbuffer;
  int hostname;
  struct hostent *host_entry;

  // retrieve hostname
  hostname = gethostname(hostbuffer, sizeof(hostbuffer));

  // retrieve host information
  host_entry = gethostbyname(hostbuffer);

  // convert an Internet network address into ASCII string
  IPbuffer = inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0]));

  // clean up and return the IPv4 address
  #if defined(_WIN32)
    WSACleanup();
  #endif

  // exit
  return IPbuffer;
}
