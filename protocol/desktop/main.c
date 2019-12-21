/*
 * This file initiates a connection from this local client to a user's Fractal
 * cloud computer through UDP hole punching, and then starts streaming the user
 * actions to the VM and receiving the video/audio stream back for display.
 *
 * Fractal Protocol version 1.1
 *
 * Last modified: 12/20/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <process.h>
  #include <windows.h>
#else
  #include <unistd.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/types.h>
  #include <sys/socket.h>
#endif

#include "../include/findip.h" // find IPv4 of host

#define BUFLEN 512 // length of buffer to receive UDP packets
#define HOLEPUNCH_SERVER_IP "34.200.170.47" // Fractal-HolePunchServer-1 on AWS Lightsail
#define HOLEPUNCH_PORT 48488 // Fractal default holepunch port
#define RECV_PORT 48800 // port on which this client listens for UDP packets

int repeat = 1; // boolean to keep the protocol going until a closing event happens

// simple struct to copy memory back
struct client {
  uint32_t ipv4;
  uint16_t port;
  char target[128]; // buflen for address
};

// simple struct with socket information for passing to threads
struct context {
  SOCKET Socket;
  struct sockaddr_in dest_addr;
  socklen_t addr_len;
};

// thread to receive the video from the VM
unsigned __stdcall ReceiveStream(void *opaque) {
  // cast the context back
  struct context recv_context = *(struct context *) opaque;

  // keep track of packets size
  int recv_size;
  char recv_buff[BUFLEN];

  // loop as long as the stream is on
  while (repeat) {
    // receive a packet
    recv_size = recvfrom(recv_context.Socket, recv_buff, sizeof(struct client), 0, (struct sockaddr *) &recv_context.dest_addr, &recv_context.addr_len);
    printf("Received packet from the VM of size: %d.\n", recv_size);
  }
  // protocol loop exited, close stream
  return 0;
}

// main client loop
int32_t main(int32_t argc, char **argv) {
  // unused argv
  (void) argv;

  // usage check
  if (argc != 1) {
    printf("Usage: client\n"); // no argument needed
    return 1;
  }

  // initialize the windows socket library if this is a windows client
  #if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
      printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
      return 2;
    }
    printf("Winsock initialized successfully.\n");
  #endif

  // environment variables
  SOCKET SENDsocket, RECVsocket; // socket file descriptors
  struct sockaddr_in recv_addr, send_addr, holepunch_addr; // addresses of endpoints
  socklen_t addr_len = sizeof(holepunch_addr); // any addr, all same len
  HANDLE ThreadHandle; // our second thread for the audio/video
  char punch_buff[sizeof(struct client)]; // buffer to receive the hole punch server reply
  int sent_size; // keep track of packets size

  // create sending UDP socket
  if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create send socket.\n");
    return 3;
  }
  printf("UDP Send socket created.\n");

  // create receiving UDP socket, this where we will receive from the VM with
  // which we are initiating the protocol
  if ((RECVsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create receive socket.\n");
    return 4;
  }
  printf("UDP Receive socket created.\n");

  // set our endpoint data to receive UDP packets
  memset(&recv_addr, 0, sizeof(recv_addr));
  recv_addr.sin_family = AF_INET;
  recv_addr.sin_port = htons(RECV_PORT); // port on which we receive UDP packets
  recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind the receive socket to our receive port address
  if (bind(RECVsocket, (struct sockaddr *) &recv_addr, sizeof(recv_addr)) < 0) {
    printf("Unable to bound socket to port %d.\n", RECV_PORT);
    return 5;
  }
  printf("UDP socket bound to port %d.\n", RECV_PORT);

  // set the hole punching server endpoint to send the first packet to initiate
  // hole punching. We will set another endpoint later for the actual server we
  // will communicate with
  memset(&holepunch_addr, 0, sizeof(holepunch_addr));
  holepunch_addr.sin_family = AF_INET;
  holepunch_addr.sin_port = htons(HOLEPUNCH_PORT);
  holepunch_addr.sin_addr.s_addr = inet_addr(HOLEPUNCH_SERVER_IP);

  // now we need to send a simple datagram to the hole punching server to let it
  // know of our public UDP endpoint. Not only the holepunch server, but the VM
  // will send their data through this endpoint. Datagram paylod is the IP
  // address of the VM with which we want to connect, normally gotten by authenticating
  // and our IPv4 so that it can be passed to the VM
  char *holepunch_message = "40.117.57.45|"; // Azure VM IP + delimitor
  char *my_ip = get_host_ipv4();
  strcat(holepunch_message, my_ip); // concatenate to get final message

  // send our endpoint and target vm IP to the hole punching server
  if (sendto(SENDsocket, holepunch_message, strlen(holepunch_message), 0, (struct sockaddr *) &holepunch_addr, addr_len) < 0) {
    printf("Unable to send client endpoint to hole punching server.\n");
    return 6;
  }
  printf("Local endpoint sent to the hole punching server.\n");

  // the hole punching server has now mapped our NAT endpoint and "punched" a
  // hole through the NAT for peers to send us direct datagrams, we now look to
  // receive the punched endpoint of the vm we connect with

  // blocking call to wait for the hole punching server to pair this client with
  // the respective VM
  recvfrom(RECVsocket, punch_buff, sizeof(struct client), 0, (struct sockaddr *) &holepunch_addr, &addr_len);
  printf("Received the endpoint of the VM from the hole punch server.\n");

  // now that we received the endpoint, we can copy it to our client struct to
  // recover the endpoint and initiate the protocol
  struct client vm; // struct to hold the endpoint
  memcpy(&vm, punch_buff, sizeof(struct client)); // copy into struct

  // now that we have the memory, we can create the endpoint we send to
  memset(&send_addr, 0, sizeof(send_addr));
  send_addr.sin_family = AF_INET;
  send_addr.sin_port = htons(vm.port); // the port to communicate with
  send_addr.sin_addr.s_addr = htonl(vm.ipv4); // the IP of the vm to send to

  // hole punching fully done, we have the info to communicate directly with the
  // VM, so we create receive video thread and start the protocol, the user actions
  // are processed directly in this thread

  // first we create the context to pass to the video/audio thread
  struct context recv_context;
  recv_context.Socket = RECVsocket;
  recv_context.dest_addr = recv_addr;
  recv_context.addr_len = addr_len;

  // launch thread #2 to start streaming the user input on this device
  ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &ReceiveStream, (void *) &recv_context, 0, NULL);

  // listens for and send user actions as long as the protocol is on
  char *message = "Hello from the client!\n";
  while (repeat) {
    // send the packet to the VM directly
    if ((sent_size = sendto(SENDsocket, message, strlen(message), 0, (struct sockaddr *) &send_addr, addr_len)) < 0) {
      printf("Failed to send user action packet to VM.\n");
    }
    printf("Sent user action packet of size %d to the VM.\n", sent_size);
  }

  // protocol loop exited, close everything
  #if defined(_WIN32)
    // thread handle
    CloseHandle(ThreadHandle);

    // sockets
    closesocket(SENDsocket);
    closesocket(RECVsocket);

    // Windows socket library
    WSACleanup();
  #else
    // if not Windows client, just close the sockets
    close(SENDsocket);
    close(RECVsocket);
  #endif

  // exit
  return 0;
}
