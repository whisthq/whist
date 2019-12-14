/*
 * This file creates a server on the host (Windows 10) VM that waits for a
 * connection request from a client, and then starts streaming its desktop video
 * and audio to the client, and receiving the user input back.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël, Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../include/fractal.h" // header file for protocol functions
#include "../include/videocapture.h"
#include "../include/encode.h"

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

// global vars and definitions
bool repeat = true; // global flag to keep streaming until client disconnects
#define RECV_BUFFER_LEN 38 // exact user input packet line to prevent clumping




typedef enum {
	videotype = 0xFA010000,
	audiotype = 0xFB010000
} Fractalframe_type_t;

typedef struct {
	Fractalframe_type_t type;
	int size;
	char data[0];
} Fractalframe_t;

#define FRAME_BUFFER_SIZE (1024 * 1024)




// main function to stream the video and audio from this server to the client
unsigned __stdcall SendStream(void *SENDsocket_param) {
  // cast the socket parameter back to socket for use
  SOCKET SENDsocket = *(SOCKET *) SENDsocket_param;

  // get window
  HWND window = NULL;
  window = GetDesktopWindow();
  frame_area frame = {0, 0, 0, 0}; // init  frame area

  // init screen capture device
  capture_device *device;
  device = create_capture_device(window, frame);

  // set encoder parameters
  int width = device->width; // in and out from the capture device
  int height = device->height; // in and out from the capture device
  int bitrate = width * 1500; // estimate bit rate based on output size

  // init encoder
  encoder_t * encoder;
  encoder = create_encoder(width, height, width, height, bitrate);

  // video variables
  int sent_size; // var to keep track of size of packets sent
  void *capturedframe; // var to hold captured frame, as a void* to RGB pixels

  // init encoded frame parameters
  Fractalframe_t *encodedframe = (Fractalframe_t *) malloc(FRAME_BUFFER_SIZE);
  memset(encodedframe, 0, FRAME_BUFFER_SIZE); // set memory to null
  encodedframe->type = videotype; // specify that this is a video frame
  size_t encoded_size; // init encoded buffer size

  // while stream is on
  while (repeat) {
    // capture a frame
    capturedframe = capture_screen(device);

    // reset encoded frame to 0 and reset buffer  before encoding
    encodedframe->size = 0;
    encoded_size = FRAME_BUFFER_SIZE - sizeof(Fractalframe_t);

    // encode captured frame into encodedframe->data
    encoder_encode(encoder, capturedframe, encodedframe->data, &encoded_size);



    printf("encoded size after: %d\n", encoded_size);


    // only send if packet is not empty
    if (encoded_size != 0) {
      // send packet
      if ((sent_size = send(SENDsocket, encodedframe->data, encoded_size, 0)) < 0) {
        // error statement if something went wrong
        printf("Socket could not send packet w/ error %d\n", WSAGetLastError());
      }
      printf("Sent packet of size %d.\n", sent_size);
    }

    // packet sent, let's reset the encoded frame memory for the next one
    memset(encodedframe, 0, FRAME_BUFFER_SIZE);
  }

  // exited while loop, stream done let's close everything
  destroy_encoder(encoder); // destroy encoder
  destroy_capture_device(device); // destroy capture device
  _endthreadex(0); // end thread and return
  return 0;
}


// main function to receive client user inputs and process them
unsigned __stdcall ReceiveClientInput(void *RECVsocket_param) {
  // cast the socket parameter back to socket for use
  SOCKET RECVsocket = *(SOCKET *) RECVsocket_param;

  int recv_size; // packet size we receive
  char *recv_buffer[RECV_BUFFER_LEN]; // buffer to receive into

  // while stream is on, listen for user messages
  while (repeat) {
    // query for packets reception indefinitely via recv until repeat set to false
    recv_size = recv(RECVsocket, recv_buffer, RECV_BUFFER_LEN, 0);
    printf("Received packet of size: %d\n", recv_size);
    printf("Message received: %s\n", recv_buffer);

    // if the packet isn't empty (aka there is an action to process)
    if (recv_size > 0) {
      // the packet we receive is the memory copy of the FractalMessage struct
      // so we memcpy it back into a FractalMessage struct
      FractalMessage fmsg = {0};
      memcpy(&fmsg, &recv_buffer, sizeof(struct FractalMessage));

      // we can now replay the user input
      ReplayUserInput(fmsg);
    }
  }
  // connection interrupted by setting repeat to false, exit protocol
  printf("Connection interrupted.\n");
  _endthreadex(0); // close thread
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

  // protocol environment variables
  SOCKET listensocket, RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in clientRECV, clientServerRECV; // file descriptors for ports
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
  HANDLE ThreadHandles[2] = {0, 0}; // array containing our 2 thread handles
  SOCKET sockets[3] = {0, 0, 0}; // array containing our socket file descriptors
  int clientServerRECV_addr_len = sizeof(struct sockaddr_in); // client address length

  // initialize server listen socket (TCP) and start listening
  listensocket = ServerInit(listensocket, config);
  sockets[0] = listensocket; // store to make destroying simpler

  // forever loop to always be listening to pick up a new connection if idle
  while (true) {
    // accept client connection once there's one on the listensocket queue
    // new active socket created on same port as listensocket,
    RECVsocket = accept(listensocket, (struct sockaddr *) &clientServerRECV, &clientServerRECV_addr_len);
    if (RECVsocket == INVALID_SOCKET) {
      // print error but keep listening to new potential connections
      printf("Accept failed with error code: %d.\n", WSAGetLastError());
    }
    else {
      // now that we got our receive socket ready to receive client input, we
      // need to create our send socket to initiate the stream
      printf("Connection accepted - Receive TCP Socket created.\n");
      sockets[1] = RECVsocket; // store to make destroying simpler





      // Creating our UDP (sending) socket
      // AF_INET = IPv4
      // SOCK_DGRAM = UDP Socket
      // IPPROTO_UDP = UDP protocol
      if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("Could not create UDP socket : %d.\n" , WSAGetLastError());
        ServerDestroy(sockets, ThreadHandles, false);
        return 5;
      }
      printf("Send UDP Socket created.\n");

      sockets[2] = SENDsocket;

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
        ServerDestroy(sockets, ThreadHandles, false);
        return 3;
      }
      printf("Connected on port: %d.\n", config.clientPortRECV);






      // time to start streaming and receiving user input
      // launch thread #1 to start streaming video & audio from server
      ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &SendStream, &SENDsocket, 0, NULL);

      // launch thread #2 to start receiving and processing client input
      ThreadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveClientInput, &RECVsocket, 0, NULL);

      // this thread, thread #0, listens for user logging out of their cloud
      // computer to stop the protocol when that happens
      while (repeat) {
        // TODO: helper function for listening to logoff event

        // user deconnected, set repeat to false to close the protocol
        //repeat = false;
      }
    }
    // client disconnected, destroy connection and restart listening
    printf("Client disconnected.\n");
    ServerDestroy(sockets, ThreadHandles, false);
  }
  // server loop exited, close everything
  ServerDestroy(sockets, ThreadHandles, true);
  return 0;
}

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
