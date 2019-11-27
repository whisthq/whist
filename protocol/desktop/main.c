/*
 * This file creates a connection on the client (Windows, MacOS, Linux) device
 * that initiates a connection to their associated server (cloud VM), which then
 * starts streaming its desktop video and audio to the client, while the client
 * starts streaming its user inputs to the server.

 Protocol version: 1.0
 Last modification: 11/25/2019

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
#else
  #include <pthread.h> // thread library on unix
  #include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static bool loggedIn = true; // global user login variable
bool repeat = true; // global flag to stream until disconnection







/*** LOGIN FUNCTIONS START ***/ /*
/*** LOGIN FUNCTIONS END ***/






/*
// main function to receive server video and audio stream and process it
int32_t ReceiveStream(SOCKET RECVsocket) {





  // initiate buffer to store the reply
  int recv_size;
  char *server_reply[2000];

  // while stream is on, listen for messages
  while (repeat) {

    //Receive a reply from the server
  	//if((recv_size = recv(RECVsocket , server_reply , 2000 , 0)) == SOCKET_ERROR)
  	//{
    //  printf("recv failed\n");
    //  return 1;
  	//}

    // we poll constantly for a message coming our way
    recv_size = recv(RECVsocket , server_reply , 2000 , 0);
    printf("Message received: %s\n", server_reply);

  }

  printf("Connection interrupted\n");






  // terminate thread as we are done with the stream
  _endthread();
  return 0;





} */

// main function to send client user inputs
int32_t SendClientInput(SOCKET SENDsocket) {






  	// init the decoder here
  	char *message = "Hey from the client!\n";
  	// then start looping, we loop while repeat is true, when we receive a disconnect
  	// request, then it becomes false
  	while (repeat) {

  		// test data send


  		if(send(SENDsocket, message, strlen(message), 0) < 0)
  		{
  			printf("Send failed\n");
  			return 1;
  		}
  		//printf("Send succeeded.\n");

      // sleep to be able to see what's happening
      Sleep(5000L);


  	}

  	printf("Repeat is false, exit\n");




  // terminate thread as we are done with the stream
  _endthread();
  return 0;






}




// add linux thread version/windows fix on both server/client
// test connection/deconnection





// main client function
int32_t main(int32_t argc, char **argv)
{
  // unused argv this is momentary until we have the login
  (void) argv;
  // variable for storing server IP address
  //char *vm_ip = "";

  // usage check for authenticating on our web servers
  if (argc != 3) {
    printf("Usage: client fractal-username fractal-password\n");
    return 1;
  }

  // user inputs for username and password
  //char *username = argv[1];
  //char *password = argv[2];

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
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings

  // initialize Winsock if this is a Windows client
  #if defined(_WIN32)
    WSADATA wsa;
    // initialize Winsock (sockets library)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
      printf("Failed. Error Code : %d.\n", WSAGetLastError());
      return 2;
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
    printf("Could not create Send TCP socket.\n");
  }
  printf("Send TCP Socket created.\n");

  // prepare the sockaddr_in structure for the send socket (server receive port)
	serverRECV.sin_family = AF_INET; // IPv4
  serverRECV.sin_addr.s_addr = inet_addr("40.117.226.121"); // VM (server) IP received from authenticating
  serverRECV.sin_port = htons(config.serverPortRECV); // initial default port 48888

	// connect client the send socket to the server receive port
	char* connect_status = connect(SENDsocket, (struct sockaddr*) &serverRECV, sizeof(serverRECV)) < 0;
	if (connect_status == SOCKET_ERROR || connect_status < 0)
	{
    printf("Could not connect to the VM (server).\n");
    return 3;
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








  // since this is a UDP socket, there is no connection necessary
  // time to start receiving the stream and sending user input
  // launch thread #1 to start streaming video & audio from server
  //_beginthread(ReceiveStream(RECVsocket), 0, NULL);

  // launch thread #2 to start sending user input
  _beginthread(SendClientInput(SENDsocket), 0, NULL);


  // add threads for linux/macos



  // keep looping until the client sends user input to disconnect, at which
  // point the server will send a packet/stop stream to say "stop sending user
  // input" and it will set the repear varto false in ReceiveStream
  while (repeat) {
    // wait one second between loops (arbitrary, these loops just wait)
    #if defined(_WIN32)
      Sleep(1000L);
    #else
      sleep(1000);
    #endif
  }
  // client or server disconnected, close everything
  // Windows case, closing sockets
  #if defined(_WIN32)
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

// renable Windows warning, if Windows client
#if defined(_WIN32)
	#pragma warning(default: 4201)
#endif
