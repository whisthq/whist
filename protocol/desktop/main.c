/*
 * This file creates a connection on the client (Windows, MacOS, Linux) device
 * that initiates a connection to their associated server (cloud VM), which then
 * starts streaming its desktop video and audio to the client, while the client
 * starts streaming its user inputs to the server.

 Protocol version: 1.0
 Last modification: 11/30/2019

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
  #pragma warning(disable: 4267) // conversion from size_t to int
#else
  #include <pthread.h> // thread library on unix
  #include <unistd.h>
#endif

#ifdef __cplusplus
//extern "C" {
#endif

// global vars and definitions
#define RECV_BUFFER_LEN 512 // max len of receive buffer
bool repeat = true; // global flag to stream until disconnection

// main thread function to receive server video and audio stream and process it
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
  // tmp unused var
  (void) argv;

  // user login status var
  static bool loggedIn = false;

  // variable for storing user credentials
  //char *credentials;
  char *user_vm_ip = "";

  // usage check for authenticating on our web servers
  if (argc != 3) {
    printf("Usage: client [username] [password]\n");
    return 1;
  }

  // user inputs for username and password
  //char *username = argv[1];
  //char *password = argv[2];

  // if the user isn't logged in currently, try to authenticate
  if (!loggedIn) {

    // TODO LATER: FIX CODE HERE ON A LOCAL WINDOWS FOR WEBSERVER QUERY
    //credentials = login(username, password);
    // print
    //printf("\r\n");
    //printf("Server response: \r\n\r\n");
    //printf(credentials);


    if (true) { // if correct credentials
      user_vm_ip = "3.90.174.193";
      loggedIn = true; // set flag as false
    }
    else {
      // incorrect username or password, couldn't authenticate
      printf("Incorrect username or password.\n");
      return 2;
    }
    // user authenticated, let's start the protocol
    printf("Successfully authenticated.\n");
  }

  // all good, we have a user and the VM IP written, time to set up the sockets
  // socket environment variables
  SOCKET RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in clientRECV, serverRECV; // this client receive port the server streams to, and the server receive port this client streams to
  int bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
  HANDLE ThreadHandles[1]; // array containing our extra thread handle
  char hexa[17] = "0123456789abcdef"; // array of hexadecimal values + null character for serializing

  // initialize Winsock if this is a Windows client
  #if defined(_WIN32)
    WSADATA wsa;
    // initialize Winsock (sockets library)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
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
  if (SENDsocket == INVALID_SOCKET || SENDsocket < 0) { // Windows & Unix cases
    // if can't create socket, return
    printf("Could not create Send TCP socket.\n");
    return 4;
  }
  printf("Send TCP Socket created.\n");

  // prepare the sockaddr_in structure for the send socket (server receive port)
	serverRECV.sin_family = AF_INET; // IPv4
  serverRECV.sin_addr.s_addr = inet_addr(user_vm_ip); // VM (server) IP received from authenticating
  serverRECV.sin_port = htons(config.serverPortRECV); // initial default port 48888

	// connect the client send socket to the server receive port (TCP)
	char *connect_status = connect(SENDsocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV));
	if (connect_status == SOCKET_ERROR || connect_status < 0) {
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
  if (RECVsocket == INVALID_SOCKET || RECVsocket < 0) { // Windows & Unix cases
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
  // now that everything is ready for the communication sockets, we need to init
  // SDL to be ready to process user inputs and display the stream
  // SDL vars for displaying
  SDL_Surface *imageSurface = NULL;
  SDL_Surface *windowSurface = NULL;
  SDL_Window *window = NULL;

  // init SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
      printf("SDL init error.\n");
      return 5;
  }
  printf("SDL initialized.\n");

  // TODO LATER: function to call to adapt window size to client

  // set the SDL window properties
  window = SDL_CreateWindow("Fractal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_W, WINDOW_H, SDL_WINDOW_ALLOW_HIGHDPI);
  windowSurface = SDL_GetWindowSurface(window);
  if (window == NULL) {
      printf("SDL window error.\n");
      return 6;
  }

  // now that SDL is ready, time to start the protocol
  // the user inputs sending is done in thread 1 (this thread)  while the video
  // receiving from the server is done in a second thread
  // launch thread #2 to start streaming video & audio from server
  ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveStream, &RECVsocket, 0, NULL);

  // all good there, time to start sending user input
  // define SDL variable to listen for user inputs
  SDL_Event msg;

  // loop indefinitely to keep sending to the server until repeat set to fasl
  while (repeat) {
    // poll for an SDL event
    if (SDL_PollEvent(&msg)) {
      // event received, define Fractal message and find which event type it is
      FractalMessage fmsg = {0};

      switch (msg.type) {
        // SDL quit event, exit switch
        case SDL_QUIT:
          repeat = false; // used clicked to close the window, close protocol
          break;
        // SDL event for keyboard key pressed or released
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          // fill Fractal message structure for sending
          fmsg.type = MESSAGE_KEYBOARD;
          fmsg.keyboard.code = (FractalKeycode) msg.key.keysym.scancode;
          fmsg.keyboard.mod = msg.key.keysym.mod;
          fmsg.keyboard.pressed = msg.key.type == SDL_KEYDOWN;
          printf("Key Code: %d\n", fmsg.keyboard.code); // print statement to see what's happening
          break;
        // SDL event for mouse location when it moves
        case SDL_MOUSEMOTION:
          fmsg.type = MESSAGE_MOUSE_MOTION;
          fmsg.mouseMotion.relative = SDL_GetRelativeMouseMode();
          fmsg.mouseMotion.x = fmsg.mouseMotion.relative ? msg.motion.xrel : msg.motion.x;
          fmsg.mouseMotion.y = fmsg.mouseMotion.relative ? msg.motion.yrel : msg.motion.y;
          printf("Mouse Position: (%d, %d)\n", fmsg.mouseMotion.x, fmsg.mouseMotion.y); // print statement to see what's happening
          break;
        // SDL event for mouse button pressed or released
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          fmsg.type = MESSAGE_MOUSE_BUTTON;
          fmsg.mouseButton.button = msg.button.button;
          fmsg.mouseButton.pressed = msg.button.type == SDL_MOUSEBUTTONDOWN;
          printf("Mouse Button Code: %d\n", fmsg.mouseButton.button); // print statement to see what's happening
          break;
        // SDL event for mouse wheel scroll
        case SDL_MOUSEWHEEL:
          fmsg.type = MESSAGE_MOUSE_WHEEL;
          fmsg.mouseWheel.x = msg.wheel.x;
          fmsg.mouseWheel.y = msg.wheel.y;
          printf("Mouse Scroll Position: (%d, %d)\n", fmsg.mouseWheel.x, fmsg.mouseWheel.y); // print statement to see what's happening
          break;

        // TODO LATER: clipboard switch case
      }
      // we broke out of the listen loop, so we have an event
      // if we have a message type identified, send event to server
      if (fmsg.type != 0) {
        // fmsg is a FractalMessage struct, so we need to serialzie it (convert
        // to string) to send it over the network
        // we first copy the memory bits to a char array
        unsigned char fmsg_char[sizeof(struct FractalMessage)];
        memcpy(fmsg_char, &fmsg, sizeof(struct FractalMessage));
        char fmsg_serialized[2 * sizeof(struct FractalMessage) + 1]; // serialized array is 2x the length since hexa



        int j;
        for (j = 0; j < sizeof(struct FractalMessage); j++) {
          printf("char #%i: %d\n", j, (int) fmsg_char[j]);
        }




        // loop over the char struct, convert each value to hexadecimal
        int i;
        for (i = 0; i < sizeof(struct FractalMessage); i++) {
          // converting to hexa
          fmsg_serialized[i * 2] = hexa[fmsg_char[i] / 16];
          fmsg_serialized[(i * 2 ) + 1] = hexa[fmsg_char[i] % 16];
        }
        printf("User action serialized, ready for sending.\n");

        // user input is serialized, ready to stream over the network
        // send data message to server
        if (send(SENDsocket, fmsg_serialized, strlen(fmsg_serialized), 0) < 0) {
          // error sending, terminate
          printf("Send failed with error code: %d\n", WSAGetLastError());
          return 7;
        }
        printf("User action sent.\n");
      }
    }
    // packet sent, let's update the SDL surface
    SDL_BlitSurface(imageSurface, NULL, windowSurface, NULL);
    SDL_UpdateWindowSurface(window);
  }
  // left the while loop, protocol received instruction to terminate
  printf("Connection interrupted.\n");

  // TODO LATER: Split this file into Windows/Unix and do threads for Unix for max efficiency

  // client or server disconnected, close everything
  // free SDL and set variables to null
  SDL_FreeSurface(imageSurface);
  SDL_FreeSurface(windowSurface);

  // terminate SDL
  SDL_DestroyWindow(window);
  SDL_Quit();

  // Windows case, closing sockets
  #if defined(_WIN32)
    // threads are done, let's close the extra handle and exit
    CloseHandle(ThreadHandles[0]);

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
  //loggedIn = false; // logout user
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
  #pragma warning(default: 4267)
#endif
