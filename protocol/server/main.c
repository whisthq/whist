/*
 * This file creates a server on the host (Windows 10) VM that waits for a
 * connection request from a client, and then starts streaming its desktop video
 * and audio to the client, and receiving the user input back.

 Protocol version: 1.0
 Last modification: 11/24/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h> // lib for socket programming on windows

#include "../include/fractal.h" // header file for protocol functions

// Winsock Library
#pragma comment(lib, "ws2_32.lib")

#if defined(_WIN32)
	#pragma warning(disable: 4201)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// main server function
int32_t main(int32_t argc, char **argv)
{
  // usage check
  if (argc > 1) {
    printf("Usage: server\n"); // no argument needed, server listens
    return 1;
  }

  // socket environment variables
	WSADATA wsa;
  SOCKET listensocket, RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in serverRECV, serverSEND, client; // this server and the client that connects
  int client_addr_len, bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings

  // initialize Winsock (sockets library)
	printf("Initialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
		return 2;
	}
	printf("Winsock Initialised.\n");

  // Creating our TCP (receiving) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // 0 = protocol automatically detected
  if ((listensocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create listen TCP socket : %d\n" , WSAGetLastError());
	}
	printf("Listen TCP Socket created.\n");

  // prepare the sockaddr_in structure for the listening socket
	serverRECV.sin_family = AF_INET; // IPv4
	serverRECV.sin_addr.s_addr = INADDR_ANY; // any IP
	serverRECV.sin_port = htons(config.serverPortRECV); // initial default port 48888

  // bind our socket to this port. If it fails, increment port by one and retry
  while (bind(listensocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV)) == SOCKET_ERROR) {
    // at most 50 attempts, after that we give up
    if (bind_attempts == 50) {
      printf("Cannot find an open port, abort.\n");
      return 3;
    }
    // display failed attempt
    printf("Bind attempt #%i failed with error code : %d\n"; , bind_attempts, WSAGetLastError());

    // increment port number and retry
    bind_attempts += 1;
    serverRECV.sin_port = htons(config.serverPortRECV + bind_attempts); // initial default port 48888
  }
  // successfully binded, we're good to go
  printf("Bind done.\n");

	// this passive socket is always open to listen for an incoming connection
	listen(listensocket , 1); // 1 maximum concurrent connection
  printf("Waiting for an incoming connection...\n");

  // forever loop to always be listening to pick up a new connection if idle
  while (true) {
    // accept client connection once there's one on the listensocket queue
    // new active socket created on same port as listensocket,
    client_addr_len = sizeof(struct sockaddr_in);
  	RECVsocket = accept(listensocket, (struct sockaddr *) &client, &client_addr_len);
  	if (RECVsocket == INVALID_SOCKET)
  	{
  		printf("Accept failed with error code: %\n", WSAGetLastError());
  	}
    else {
      // now that we got our receive socket ready to receive client input, we
      // need to create our send socket to initiate the stream
      printf("Connection accepted\n");

      // Creating our UDP (sending) socket
      // AF_INET = IPv4
      // SOCK_DGRAM = UDP Socket
      // 0 = protocol automatically detected
      if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    	{
    		printf("Could not create UDP socket : %d\n" , WSAGetLastError());
    	}
    	printf("UDP Socket created.\n");

      // to bind to the client receiving port, we need its IP and port, which we
      // can get since we have accepted connection on our receiving port
      char *client_ip = inet_ntoa(client.sin_addr);

      // prepare the sockaddr_in structure for the sending socket
    	serverSEND.sin_family = AF_INET; // IPv4
    	serverSEND.sin_addr.s_addr = client_ip; // client IP to send stream to
    	serverSEND.sin_port = htons(config.clientPortRECV); // client port to send stream to

      // since this is a UDP socket, there is no connection necessary
      // time to start streaming and receiving user input
      while (true) {










            /*
            this is the loop where the user input reception and stream will
            take place, using the select() command to select between the two
            ports

            //Send some data
          	message = &quot;GET / HTTP/1.1\r\n\r\n&quot;;
          	if( send(s , message , strlen(message) , 0) &lt; 0)
          	{
          		puts(&quot;Send failed&quot;);
          		return 1;
          	}
          	puts(&quot;Data Send\n&quot;);

          	return 0;

            */






      }
      // client disconnected, close the send socket since it's client specific
      closesocket(SENDsocket);
    }
    // client disconnected, restart listening for connections
    printf("Client disconnected.\n");
  }
  // server loop exited, close everything
  closesocket(RECVsocket);
  closesocket(listensocket);
  WSACleanup(); // close Windows socket library

	return 0;
}

#ifdef __cplusplus
}
#endif

// renable Windows warning
#if defined(_WIN32)
	#pragma warning(default: 4201)
#endif
