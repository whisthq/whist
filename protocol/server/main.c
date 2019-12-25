/*
 * This file turns a host (Windows 10) VM into a server that gets connected to
 * a local client via UDP hole punching, and then starts streaming its desktop
 * video and audio to the client, and receiving the user actions back.
 *
 * Fractal Protocol version 1.1
 *
 * Last modified: 12/24/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/findip.h" // find IPv4 of host
#include "../include/socket.h"

#define BUFLEN 512 // length of buffer to receive UDP packets
#define HOLEPUNCH_SERVER_IP "34.200.170.47" // Fractal-HolePunchServer-1 on AWS Lightsail
#define HOLEPUNCH_PORT 48488 // Fractal default holepunch port
#define RECV_PORT 48801 // port on which this client listens for UDP packets

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

// thread to send the audio/video of this VM to the local client
unsigned __stdcall SendStream(void *opaque) {
  // cast the context back
  struct context send_context = *(struct context *) opaque;

  // keep track of packets size
  int sent_size;
  char *message = "Hello from the VM!\n";

  // listens for and send user actions as long as the protocol is on
  while (repeat) {
    // send the packet to the VM directly
    if ((sent_size = sendto(send_context.Socket, message, strlen(message), 0, (struct sockaddr *) &send_context.dest_addr, send_context.addr_len)) < 0) {
      // printf("Failed to send frame packet to VM.\n");
    }
    // printf("Sent frame packet of size %d to the VM.\n", sent_size);
  }
  // protocol loop exited, close stream
  return 0;
}

// thread to receive and replay the local client actions on the VM
unsigned __stdcall ReceiveUserActions(void *opaque) {
  // cast the context back
  struct context recv_context = *(struct context *) opaque;

  // keep track of packets size
  int recv_size;
  char recv_buff[BUFLEN];

  // loop as long as the stream is on
  while (repeat) {
    // receive a packet
    recv_size = recvfrom(recv_context.Socket, recv_buff, BUFLEN, 0, (struct sockaddr *) &recv_context.dest_addr, &recv_context.addr_len);
    // printf("Received packet from the local client of size: %d.\n", recv_size);
  }
  // protocol loop exited, close stream
  return 0;
}

// main server loop
int main() {
  // initialize the windows socket library if this is a windows client
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
    printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
    return -1;
  }
  printf("Winsock initialized successfully.\n");

  // environment variables
  SOCKET SENDsocket, RECVsocket; // socket file descriptors
  struct sockaddr_in recv_addr, send_addr, holepunch_addr; // addresses of endpoints
  socklen_t addr_len = sizeof(holepunch_addr); // any addr, all same len
  HANDLE ThreadHandles[2]; // our two other threads for the audio/video and user actions
  char punch_buff[sizeof(struct client)]; // buffer to receive the hole punch server reply
  int sent_size; // keep track of packets size

  // create sending UDP socket
  if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create send socket with error code: %d.\n", WSAGetLastError());
    return -2;
  }
  printf("UDP Send socket created.\n");

  // create receiving UDP socket, this where we will receive from the local
  // client with which we are initiating the protocol
  if ((RECVsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create receive socket with error code: %d.\n", WSAGetLastError());
    return -3;
  }
  printf("UDP Receive socket created.\n");

  // set our endpoint data to receive UDP packets
  memset(&recv_addr, 0, sizeof(recv_addr));
  recv_addr.sin_family = AF_INET;
  recv_addr.sin_port = htons(RECV_PORT); // port on which we receive UDP packets
  recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind the receive socket to our receive port address
  if (bind(RECVsocket, (struct sockaddr *) &recv_addr, sizeof(recv_addr)) < 0) {
    printf("Unable to bound socket to port %d with error code: %d.\n", RECV_PORT, WSAGetLastError());
    return -4;
  }
  printf("UDP Receive Socket bound to port %d.\n", RECV_PORT);

  // forever loop to automatically re-initiate the hole punching and be ready
  // to pick up a new user as soon as a user logs out
  while (1) {
    // set the hole punching server endpoint to send the first packet to initiate
    // hole punching. We will set another endpoint later for the actual server we
    // will communicate with
    memset(&holepunch_addr, 0, sizeof(holepunch_addr));
    holepunch_addr.sin_family = AF_INET;
    holepunch_addr.sin_port = htons(HOLEPUNCH_PORT);
    holepunch_addr.sin_addr.s_addr = inet_addr(HOLEPUNCH_SERVER_IP);

    // now we need to send a simple datagram to the hole punching server to let it
    // know of our public UDP endpoint. Since this is a "server" (VM) being connected
    // to, it doesn't know the IP of the client it gets connected with so we will
    // just send a packet with our own IPv4 for the client to connect to
    char *punch_message = "40.117.57.45"; // hardcoded for this VM, TODO: replace this by webserver querying
    strcat(punch_message, "S"); // add client tag to let the hole punch server know this is from a VM/server

    // send our endpoint to the hole punching server
    // NOTE: we send with the RECVsocket so that the hole punch servers maps the port of the RECV socket to
    // then send to it, but will use the SENDsocket to send to the local client after hole punching is done
    if (reliable_udp_sendto(RECVsocket, punch_message, strlen(punch_message), holepunch_addr, addr_len) < 0) {
      printf("Unable to send VM endpoint to hole punching server w/ error code: %d.\n", WSAGetLastError());
      return -5;
    }
    printf("Local endpoint sent to the hole punching server.\n");

    // the hole punching server has now mapped our NAT endpoint and "punched" a
    // hole through the NAT for peers to send us direct datagrams, we now look to
    // receive the punched endpoint of the local client we connect with
    printf("Waiting for a client to connect and request this VM...\n");
    Sleep(2000L); // sleep two seconds to ensure no packet clumping








    // blocking call to wait for the hole punching server to pair this VM with
    // the local client that requested connection
    reliable_udp_recvfrom(RECVsocket, punch_buff, BUFLEN, holepunch_addr, addr_len);
    printf("Received the endpoint of the local client from the hole punch server.\n");

    // now that we received the endpoint, we can copy it to our client struct to
    // recover the endpoint and initiate the protocol
    struct client local_client; // struct to hold the endpoint
    memcpy(&local_client, punch_buff, sizeof(struct client)); // copy into struct

    // now that we have the memory, we can create the endpoint we send to
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = local_client.port; // the port to communicate with, already in byte network order
    send_addr.sin_addr.s_addr = local_client.ipv4; // the IP of the local client to send to, already in byte network order

    // hole punching fully done, we have the info to communicate directly with the
    // local client, so we create the two threads to process the audio/video and
    // the user actions and start the protocol
    repeat = 1; // new client found, set the protocol to on

    // first we create the context to pass to the user actions thread
    struct context recv_context;
    recv_context.Socket = RECVsocket;
    recv_context.dest_addr = recv_addr;
    recv_context.addr_len = addr_len;

    // and the context to pass to the video/audio thread
    struct context send_context;
    send_context.Socket = SENDsocket;
    send_context.dest_addr = send_addr;
    send_context.addr_len = addr_len;

    // launch thread #2 to stream the video/audio of this VM to the local client
    ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &SendStream, (void *) &send_context, 0, NULL);

    // launch thread #3 to receive and replay actions done on the local client
    ThreadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveUserActions, (void *) &recv_context, 0, NULL);

    // in this thread, we listen for a user trying to shut down/disconnect from
    // the VM, to stop the stream and keep the VM turned on if that happens, to be
    // ready to pick up a new user right away
    while (repeat) {
      // TODO: listen for Windows logout functions
    }
    // this user logged off, since repeat is false, so we go back to waiting for a new user
  }
  // protocol loop fully exited, shut down
  // thread handles
  CloseHandle(ThreadHandles[0]);
  CloseHandle(ThreadHandles[1]);

  // sockets
  closesocket(SENDsocket);
  closesocket(RECVsocket);

  // Windows socket library
  WSACleanup();

  // exit
  return 0;
}
