/*
 * This file creates a connection on the client (Windows, MacOS, Linux) device
 * that initiates a connection to their associated server (cloud VM), which then
 * starts streaming its desktop video and audio to the client, while the client
 * starts streaming its user inputs to the server.

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

#include "../include/fractal.h" // header file for protocol functions
#include "../include/webserver.h" // header file for webserver query functions

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

#define MSG_CLIPBOARD 7

#define WINDOW_W 1280
#define WINDOW_H 720

// include Winsock library & disable warning if on Windows client
// include pthread library for unix threads if on linux/macos
#if defined(_WIN32)
  #include <winsock2.h> // lib for socket programming on windows
  #include <windows.h> // general windows lib
  #pragma warning(disable: 4201)
  #pragma warning(disable: 4024) // disable thread warning
  #pragma warning(disable: 4113) // disable thread warning type
  #pragma warning(disable: 4244) // disable u_int to u_short conversion warning
  #pragma warning(disable: 4047) // disable char * indirection levels warning
  #pragma warning(disable: 4477) // printf type warning
#else
  #include <pthread.h> // thread library on unix
  #include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// global vars and definitions
#define RECV_BUFFER_LEN 512 // max len of receive buffer
static bool loggedIn = false; // global user login variable
bool repeat = true; // global flag to stream until disconnection

// main function to send client user inputs
unsigned __stdcall SendClientInput(void *SENDsocket_param) {
    // cast the socket parameter back to socket for use
  	SOCKET SENDsocket = *(SOCKET *) SENDsocket_param;

    // message to send to the server
  	char *message = "Hey from the client!";
    int sendsize;

    // loop indefinitely to keep sending to the server until repeat set to fasl
  	while (repeat) {
      // send data message to server
  		if ((sendsize = send(SENDsocket, message, strlen(message), 0)) < 0) {
        // error, terminate thread and exit
  			printf("Send failed with error code: %d\n", WSAGetLastError());
        _endthreadex(0);
  			return 1;
  		}
      // 5 seconds sleep to see what's happening in the terminal
      Sleep(5000L);
  	}
    // connection interrupted by setting repeat to false, exit protocol
    printf("Connection interrupted.\n");

  	// terminate thread as we are done with the stream
  	_endthreadex(0);
  	return 0;
}

// main function to receive server video and audio stream and process it
unsigned __stdcall ReceiveStream(void *RECVsocket_param) {
  // cast the socket parameter back to socket for use
	SOCKET RECVsocket = *(SOCKET *) RECVsocket_param;

  // initiate buffer to store the reply
  int recv_size;
  char *server_reply[RECV_BUFFER_LEN];

  // while stream is on, listen for messages
  while (repeat) {
    // query for packets reception indefinitely via recv until repeat set to false
    recv_size = recv(RECVsocket, server_reply, RECV_BUFFER_LEN, 0);
    printf("Message received: %s\n", server_reply);
  }
  // connection interrupted by setting repeat to false, exit protocol
  printf("Connection interrupted.\n");

	// terminate thread as we are done with the stream
	_endthreadex(0);
	return 0;
}

// main client function
int32_t main(int32_t argc, char **argv) {
  // variable for storing user credentials
  char *credentials;
  char *user_vm_ip;

  // usage check for authenticating on our web servers
  if (argc != 3) {
    printf("Usage: client [username] [password]\n");
    return 1;
  }

  // user inputs for username and password
  char *username = argv[1];
  char *password = argv[2];

  // if the user isn't logged in currently, try to authenticate
  if (!loggedIn) {
    credentials = login(username, password);


    printf("\r\n");
    printf("Server response: \r\n\r\n");
    printf(credentials);

    // if correct credentials
    // authenticate and store the VM IP
    // else exit

    return 2;
    /*
    if () {

    }
    else {
      // incorrect username or password, couldn't authenticate
      printf("Incorrect username or password.\n");
      return 2;
    }
    // user authenticated, let's start the protocol
    printf("Successfully authenticated.\n");
    */
  }







  // query Fractal web servers to authenticate the user and get the server IP
  //vm_ip = login(username, password);
  //if (!loggedIn || vm_ip == "")
  //{
  //  printf("Could not authenticate user, invalid username or password\n");
  //  return 2;
  // }






  // all good, we have a user and the VM IP written, time to set up the sockets
  // socket environment variables
  SOCKET RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in clientRECV, serverRECV; // this client receive port the server streams to, and the server receive port this client streams to
  int bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
  HANDLE ThreadHandles[2]; // array containing our 2 thread handles

  // initialize Winsock if this is a Windows client
  #if defined(_WIN32)
    WSADATA wsa;
    // initialize Winsock (sockets library)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
      printf("Failed. Error Code : %d.\n", WSAGetLastError());
      return 3;
    }
    printf("Winsock Initialised.\n");
  #endif

  // Creating our TCP (sending) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // IPPROTO_TCP = TCP protocol
  SENDsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (SENDsocket == INVALID_SOCKET || SENDsocket < 0) // Windows & Unix cases
  {
    // if can't create socket, return
    printf("Could not create Send TCP socket.\n");
    return 4;
  }
  printf("Send TCP Socket created.\n");

  // prepare the sockaddr_in structure for the send socket (server receive port)
	serverRECV.sin_family = AF_INET; // IPv4
  serverRECV.sin_addr.s_addr = inet_addr("40.117.226.121"); // VM (server) IP received from authenticating
  serverRECV.sin_port = htons(config.serverPortRECV); // initial default port 48888

	// connect the client send socket to the server receive port (TCP)
	char* connect_status = connect(SENDsocket, (struct sockaddr*) &serverRECV, sizeof(serverRECV));
	if (connect_status == SOCKET_ERROR || connect_status < 0)
	{
    printf("Could not connect to the VM (server).\n");
    return 5;
	}
  printf("Connected.\n");

  // now that we're connected, we need to create our receiving UDP socket
  // Creating our UDP (receiving) socket
  // AF_INET = IPv4
  // SOCK_DGAM = UDP Socket
  // IPROTO_UDP = UDP protocol
  RECVsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (RECVsocket == INVALID_SOCKET || RECVsocket < 0) // Windows & Unix cases
  {
    printf("Could not create Receive UDP socket.\n");
  }
  printf("Receive UDP Socket created.\n");

  // prepare the sockaddr_in structure for the receiving socket
  clientRECV.sin_family = AF_INET; // IPv4
  clientRECV.sin_addr.s_addr = INADDR_ANY; // any IP
  clientRECV.sin_port = htons(config.clientPortRECV); // initial default port 48888

  // for the recv/recvfrom function to work, we need to bind the socket even if it is UDP
  // bind our socket to this port. If it fails, increment port by one and retry
  while (bind(RECVsocket, (struct sockaddr *) &clientRECV, sizeof(clientRECV)) == SOCKET_ERROR) {
    // at most 50 attempts, after that we give up
    if (bind_attempts == 50) {
      printf("Cannot find an open port, abort.\n");
      return 4;
    }
    // display failed attempt
    printf("Bind attempt #%i failed with error code : %d.\n", bind_attempts, WSAGetLastError());

    // increment port number and retry
    bind_attempts += 1;
    clientRECV.sin_port = htons(config.clientPortRECV + bind_attempts); // initial default port 48888
  }
  // successfully binded, we're good to go
  printf("Bind done on port: %d.\n", ntohs(clientRECV.sin_port));

  // since this is a UDP socket, there is no connection necessary
  // time to start receiving the stream and sending user input
  // launch thread #1 to start streaming video & audio from server
  ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveStream, &RECVsocket, 0, NULL);

  // launch thread #2 to start sending user input
  ThreadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, &SendClientInput, &SENDsocket, 0, NULL);

  // TODO LATER: Add a third thread that listens for disconnect and sets repeat=false
  // TODO LATER: Split this file into Windows/Unix and do threads for Unix for max efficiency

  // block until our 2 threads terminate, so until the protocol terminates
  WaitForMultipleObjects(2, ThreadHandles, true, INFINITE);

  // client or server disconnected, close everything
  // Windows case, closing sockets
  #if defined(_WIN32)
    // threads are done, let's close their handles and exit
    CloseHandle(ThreadHandles[0]);
    CloseHandle(ThreadHandles[1]);

    // close the sockets
    closesocket(RECVsocket);
    closesocket(SENDsocket);
    WSACleanup(); // close Windows socket library
  #else
    close(RECVsocket);
    close(SENDsocket);
  #endif

  // write close time to server, set loggedin to false and return
  //logout(username);
  //loggedIn = false;
  return 0;
}

#ifdef __cplusplus
}
#endif

// re-enable Windows warning, if Windows client
#if defined(_WIN32)
	#pragma warning(default: 4201)
  #pragma warning(default: 4024)
  #pragma warning(default: 4113)
  #pragma warning(default: 4244)
  #pragma warning(default: 4047)
  #pragma warning(default: 4477)
#endif
