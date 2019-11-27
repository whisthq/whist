/*
 * This file creates a server on the host (Windows 10) VM that waits for a
 * connection request from a client, and then starts streaming its desktop video
 * and audio to the client, and receiving the user input back.

 Protocol version: 1.0
 Last modification: 11/27/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <Ws2tcpip.h> // other Windows socket library (of course, thanks #Microsoft)
#include <winsock2.h> // lib for socket programming on windows
#include <process.h> // for threads programming

#include "../include/fractal.h" // header file for protocol functions

// Winsock Library
#pragma comment(lib, "ws2_32.lib")

#if defined(_WIN32)
	#pragma warning(disable: 4201)
	#pragma warning(disable: 4024) // disable thread warning
	#pragma warning(disable: 4113) // disable thread warning type
	#pragma warning(disable: 4244) // disable u_int to u_short conversion warning
	#pragma warning(disable: 4047) // disable char * indirection levels warning
	#pragma warning(disable: 4701) // potentially unitialized var
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool repeat = true; // global flag to keep streaming until client disconnects





// main function to stream the video and audio from this server to the client
int32_t SendStream(SOCKET SENDsocket, struct sockaddr_in clientRECV)
{









	char message[512] = "Hey from the server!\n";
	int slen = sizeof(clientRECV);

	int sent;


	// init the decoder here
	// then start looping, we loop while repeat is true, when we receive a disconnect
	// request, then it becomes false
	while (repeat) {

		// test data send


		//send the message
		if ((sent = sendto(SENDsocket, message, strlen(message), 0, (struct sockaddr *) &clientRECV, slen)) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			return 11;
		}
		printf("Sent! Sending #bytes = %d to port: %d\n", sent, clientRECV.sin_port);


		//printf("Send succeeded.\n");

		// sleep so we can see whats up
		Sleep(5000L);

	}

	printf("Repeat is false, exit\n");







	// terminate thread as we are done with the stream
	_endthread();
	return 0;
}




// main function to receive client user inputs and process them
int32_t ReceiveClientInput(SOCKET RECVsocket)
{
	// initiate buffer to store the reply
  int recv_size;
  char *client_reply[2000];
  // while stream is on, listen for messages
  while (repeat) {
		// need recv to run indefinitely until repeat over otherwise it mighttry to
		// receive before send happened
    //Receive a reply from the server
  	//if((recv_size = recv(RECVsocket , client_reply , 2000 , 0)) == SOCKET_ERROR)
  	//{
    //  printf("recv failed\n");
    //  return 1;
  	//}
		// we constantly poll for messages coming our way
		recv_size = recv(RECVsocket , client_reply , 2000 , 0);
    printf("Message received: %s\n", client_reply);
  }
  printf("Connection interrupted\n");
	// terminate thread as we are done with the stream
	_endthread();
	return 0;
}

// main server function
int32_t main(int32_t argc, char **argv)
{
   // unused argv
   (void) argv;

  // usage check
  if (argc > 1) {
    printf("Usage: server\n"); // no argument needed, server listens
    return 1;
  }

  // socket environment variables
  WSADATA wsa;
  SOCKET listensocket, RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in serverRECV, clientRECV, clientServerRECV; // this server receive port and the client receive port, clientServerRECV is the same sockaddr_in as serverRECV, but sent from the client when it connects
  int clientServerRECV_addr_len, bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings

  // initialize Winsock (sockets library)
  printf("Initialising Winsock...\n");
  if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
  {
	  printf("Failed. Error Code : %d.\n", WSAGetLastError());
	  return 2;
  }
  printf("Winsock Initialised.\n");

  // Creating our TCP (receiving) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // 0 = protocol automatically detected
  if ((listensocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create listen TCP socket : %d.\n" , WSAGetLastError());
		return 3;
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
      return 4;
    }
    // display failed attempt
    printf("Bind attempt #%i failed with error code : %d.\n", bind_attempts, WSAGetLastError());

    // increment port number and retry
    bind_attempts += 1;
    serverRECV.sin_port = htons(config.serverPortRECV + bind_attempts); // initial default port 48888
  }
  // successfully binded, we're good to go
  printf("Bind done.\n");

	// this passive socket is always open to listen for an incoming connection
	listen(listensocket, 1); // 1 maximum concurrent connection
  printf("Waiting for an incoming connection...\n");

  // forever loop to always be listening to pick up a new connection if idle
  while (true) {
    // accept client connection once there's one on the listensocket queue
    // new active socket created on same port as listensocket,
    clientServerRECV_addr_len = sizeof(struct sockaddr_in);
  	RECVsocket = accept(listensocket, (struct sockaddr *) &clientServerRECV, &clientServerRECV_addr_len);
  	if (RECVsocket == INVALID_SOCKET)
  	{
  		printf("Accept failed with error code: %d.\n", WSAGetLastError());
			return 5;
  	}
    else {
      // now that we got our receive socket ready to receive client input, we
      // need to create our send socket to initiate the stream
      printf("Connection accepted - Receive TCP Socket created.\n");

      // Creating our UDP (sending) socket
      // AF_INET = IPv4
      // SOCK_DGRAM = UDP Socket
      // IPPROTO_UDP = UDP protocol
			if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
    	{
    		printf("Could not create UDP socket : %d.\n" , WSAGetLastError());
				return 6;
    	}
    	printf("Send UDP Socket created.\n");

      // to bind to the client receiving port, we need its IP and port, which we
      // can get since we have accepted connection on our receiving port
      char *client_ip = inet_ntoa(clientServerRECV.sin_addr);

      // prepare the sockaddr_in structure for the sending socket
    	clientRECV.sin_family = AF_INET; // IPv4
    	clientRECV.sin_addr.s_addr = client_ip; // client IP to send stream to
      clientRECV.sin_port = htons(config.clientPortRECV); // client port to send stream to, 48888


      // since this is a UDP socket, there is no connection necessary
      // time to start streaming and receiving user input
			// launch thread #1 to start streaming video & audio from server
	    _beginthread(SendStream(SENDsocket, clientRECV), 0, NULL);

			// launch thread #2 to start receiving and processing client input
		//	_beginthread(ReceiveClientInput(RECVsocket), 0, NULL);



			// keep looping until client disconnects
			// repeat var modified in ReceiveClientInput when no more input received
      while (repeat) {
				// wait one second between loops (arbitrary, these loops just wait)
				Sleep(1000L);
      }
			// exited the while loop because repeat set false in ReceiveClientInput
			// function after we stopped receiving user keepalive, so presumably the
			// client is disconnected and we stop everything
    }
		// client disconnected, restart listening for connections
    printf("Client disconnected.\n");

		// client disconnected, close the send socket since it's client specific
		// threads already closed from within their respective function
		closesocket(SENDsocket);
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
	#pragma warning(default: 4024)
	#pragma warning(default: 4113)
	#pragma warning(default: 4244)
	#pragma warning(default: 4047)
	#pragma warning(default: 4701)
#endif
