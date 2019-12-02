/*
 * This file creates a server on the host (Windows 10) VM that waits for a
 * connection request from a client, and then starts streaming its desktop video
 * and audio to the client, and receiving the user input back.

 Protocol version: 1.0
 Last modification: 11/30/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <ws2tcpip.h> // other Windows socket library (of course, thanks #Microsoft)
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
	#pragma warning(disable: 4477) // printf type warning
	#pragma warning(disable: 4267) // conversion from size_t to int
	#pragma warning(disable: 4996) // strncpy unsafe warning
#endif

#ifdef __cplusplus
extern "C" {
#endif

// global vars and definitions
#define RECV_BUFFER_LEN 33 // our protocol sends packets of len 33, this prevents two packets clumping together in the socket buffer
bool repeat = true; // global flag to keep streaming until client disconnects

// main function to stream the video and audio from this server to the client
unsigned __stdcall SendStream(void *SENDsocket_param) {
	// cast the socket parameter back to socket for use
	SOCKET SENDsocket = *(SOCKET *) SENDsocket_param;

	// message to send to the client
	char *message = "Hey from the server!";
	int sent_size; // size of data sent

	// loop indefinitely to keep sending to the client until repeat set to fasle
	while (repeat) {
		// send data message to client
		if ((sent_size = send(SENDsocket, message, strlen(message), 0)) == SOCKET_ERROR) {
			// error, terminate thread and exit
			printf("Send failed, terminate stream.\n");
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

// main function to receive client user inputs and process them
unsigned __stdcall ReceiveClientInput(void *RECVsocket_param) {
	// cast the socket parameter back to socket for use
	SOCKET RECVsocket = *(SOCKET *) RECVsocket_param;

	// initiate buffer to store the user action that we will receive
  int recv_size;
  char *client_action_buffer[RECV_BUFFER_LEN];
	char hexa[] = "0123456789abcdef"; // array of hexadecimal values + null character for deserializing

	// while stream is on, listen for messages
	while (repeat) {
	  // query for packets reception indefinitely via recv until repeat set to false
	  recv_size = recv(RECVsocket, client_action_buffer, RECV_BUFFER_LEN, 0);
	  printf("Received packet of size: %d\n", recv_size);
	  printf("Message received: %s\n", client_action_buffer);

	  // if the packet isn't empty (aka there is an action to process)
	  if (recv_size != 0) {
	    // the packet we receive is the FractalMessage struct serialized to hexadecimal,
	    // we need to deserialize it to feed it to the Windows API
	    unsigned char fmsg_char[sizeof(struct FractalMessage)]; // array to hold the hexa values in char (decimal) format

	    // first, we need to copy it to a char[] for it to be iterable
	    char iterable_buffer[RECV_BUFFER_LEN] = "";
	    strncpy(iterable_buffer, client_action_buffer, RECV_BUFFER_LEN);

	    // now we iterate over the length of the FractalMessage struct and fill an
	    // array with the decimal value conversion of the hex we received
	    int i, index_0, index_1; // tmp
	    for (i = 0; i < sizeof(struct FractalMessage); i++) {
	      // find index of the two characters for the current hexadecimal value
	    index_0 = strchr(hexa, iterable_buffer[i * 2]) - hexa;
	    index_1 = strchr(hexa, iterable_buffer[(i * 2) + 1]) - hexa;

	    // now convert back to decimal and store in array
	    fmsg_char[i] = index_0 * 16 + index_1; // conversion formula
	    }
	    // now that we got the de-serialized memory values of the user input, we
	    // can copy it back to a FractalMessage struct
	    FractalMessage fmsg = {0};
	    memcpy(&fmsg, &fmsg_char, sizeof(struct FractalMessage));

			// all good, we're back with our user input to replay
			printf("User action deserialized, ready for replaying.\n");

			// time to create an input event for the windows API based on our Fractal event
			INPUT Event = {0};





			//
			switch (fmsg.type) {
				// Windows event for keyboard action
				case MESSAGE_KEYBOARD:
					Event.type = INPUT_KEYBOARD;
					Event.ki.wScan = fmsg.keyboard.code;
					Event.ki.time = 0; // system supplies timestamp




					if (fmsg.keyboard.pressed) {
					Event.ki.dwFlags = KEYEVENTF_SCANCODE;
					} else {
						Event.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
					}
					break;

				case MESSAGE_MOUSE_MOTION:


					Event.type = INPUT_MOUSE;
					Event.mi.dx = fmsg.mouseMotion.x;
					Event.mi.dy = fmsg.mouseMotion.y;
					Event.mi.mouseData = 0;
					Event.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
					break;


				case  MESSAGE_MOUSE_BUTTON:


					break;


				case MESSAGE_MOUSE_WHEEL:

					break;

			}




			// now that we have mapped our FMSG to a Windows event, let's play it
			SendInput(1, &Event, sizeof(INPUT)); // 1 structure to send
		}
  }
	// connection interrupted by setting repeat to false, exit protocol
  printf("Connection interrupted.\n");

	// terminate thread as we are done with the stream
	_endthreadex(0);
	return 0;
}

// main server function
int32_t main(int32_t argc, char **argv) {
   // unused argv
   (void) argv;

  // usage check
  if (argc != 1) {
    printf("Usage: server\n"); // no argument needed, server listens
    return 1;
  }

  // socket environment variables
  WSADATA wsa;
  SOCKET listensocket, RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in serverRECV, clientRECV, clientServerRECV; // file descriptors for the ports, clientRECV = clientServerRECV
	int clientServerRECV_addr_len, bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
	HANDLE ThreadHandles[2]; // array containing our 2 thread handles

  // initialize Winsock (sockets library)
  printf("Initialising Winsock...\n");
  if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
	  printf("Failed. Error Code : %d.\n", WSAGetLastError());
	  return 2;
  }
  printf("Winsock Initialised.\n");

  // Creating our TCP (receiving) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // 0 = protocol automatically detected
  if ((listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		printf("Could not create listen TCP socket: %d.\n" , WSAGetLastError());
		WSACleanup(); // close Windows socket library
		return 3;
	}
	printf("Listen TCP Socket created.\n");

  // prepare the sockaddr_in structure for the listening socket
	serverRECV.sin_family = AF_INET; // IPv4
	serverRECV.sin_addr.s_addr = INADDR_ANY; // any IP
	serverRECV.sin_port = htons(config.serverPortRECV); // initial default port

  // bind our socket to this port. If it fails, increment port by one and retry
  while (bind(listensocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV)) == SOCKET_ERROR) {
    // at most 50 attempts, after that we give up
    if (bind_attempts == 50) {
      printf("Cannot find an open port, abort.\n");
			closesocket(listensocket); // close open socket
			WSACleanup(); // close Windows socket library
      return 4;
    }
    // display failed attempt
    printf("Bind attempt #%i failed with error code: %d.\n", bind_attempts, WSAGetLastError());

    // increment port number and retry
    bind_attempts += 1;
    serverRECV.sin_port = htons(config.serverPortRECV + bind_attempts); // initial default port 48888
  }
  // successfully binded, we're good to go
  printf("Bind done on port: %d.\n", ntohs(serverRECV.sin_port));

	// this passive socket is always open to listen for an incoming connection
	listen(listensocket, 1); // 1 maximum concurrent connection
  printf("Waiting for an incoming connection...\n");

  // forever loop to always be listening to pick up a new connection if idle
  while (true) {
    // accept client connection once there's one on the listensocket queue
    // new active socket created on same port as listensocket,
    clientServerRECV_addr_len = sizeof(struct sockaddr_in);
  	RECVsocket = accept(listensocket, (struct sockaddr *) &clientServerRECV, &clientServerRECV_addr_len);
  	if (RECVsocket == INVALID_SOCKET) {
			// print error but keep listening to new potential connections
  		printf("Accept failed with error code: %d.\n", WSAGetLastError());
  	}
    else {
      // now that we got our receive socket ready to receive client input, we
      // need to create our send socket to initiate the stream
      printf("Connection accepted - Receive TCP Socket created.\n");

      // Creating our UDP (sending) socket
      // AF_INET = IPv4
      // SOCK_DGRAM = UDP Socket
      // IPPROTO_UDP = UDP protocol
			if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
    		printf("Could not create UDP socket : %d.\n" , WSAGetLastError());
				closesocket(listensocket); // close open socket
				closesocket(RECVsocket); // close open socket
				WSACleanup(); // close Windows socket library
				return 5;
    	}
    	printf("Send UDP Socket created.\n");

      // to bind to the client receiving port, we need its IP and port, which we
      // can get since we have accepted connection on our receiving port
      char *client_ip = inet_ntoa(clientServerRECV.sin_addr);

      // prepare the sockaddr_in structure for the sending socket
    	clientRECV.sin_family = AF_INET; // IPv4
    	clientRECV.sin_addr.s_addr = inet_addr(client_ip); // client IP to send stream to
      clientRECV.sin_port = htons(config.clientPortRECV); // client port to send stream to

			// since this is a UDP socket, there is no connection necessary
			// however, we do a connect to force only this specific server IP onto the
			// client socket to ensure our stream will stay intact
			// connect the server send socket to the client receive port (UDP)
			char *connect_status = connect(SENDsocket, (struct sockaddr *) &clientRECV, sizeof(clientRECV));
			if (connect_status == SOCKET_ERROR) {
		    printf("Could not connect to the client w/ error: %d\n", WSAGetLastError());
			  closesocket(listensocket); // close open socket
				closesocket(RECVsocket); // close open socket
				closesocket(SENDsocket); // close open socket
			  WSACleanup(); // close Windows socket library
		    return 3;
			}
		  printf("Connected on port: %d.\n", config.clientPortRECV);

      // time to start streaming and receiving user input
			// launch thread #1 to start streaming video & audio from server
			ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &SendStream, &SENDsocket, 0, NULL);

			// launch thread #2 to start receiving and processing client input
			ThreadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveClientInput, &RECVsocket, 0, NULL);

			// TODO LATER: Add a third thread that listens for disconnect and sets repeat=false

			// block until our 2 threads terminate, so until the protocol terminates
			WaitForMultipleObjects(2, ThreadHandles, true, INFINITE);

			// threads are done, let's close their handles and exit
			CloseHandle(ThreadHandles[0]);
			CloseHandle(ThreadHandles[1]);
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

// re-enable Windows warnings
#if defined(_WIN32)
	#pragma warning(default: 4201)
	#pragma warning(default: 4024)
	#pragma warning(default: 4113)
	#pragma warning(default: 4244)
	#pragma warning(default: 4047)
	#pragma warning(default: 4701)
	#pragma warning(default: 4477)
	#pragma warning(default: 4267)
	#pragma warning(default: 4996)
#endif
