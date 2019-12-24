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
  #include <errno.h>
#endif

#include "../include/findip.h" // find IPv4 of host
#include "../include/socket.h"

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
    recv_size = recvfrom(recv_context.Socket, recv_buff, BUFLEN, 0, (struct sockaddr *) &recv_context.dest_addr, &recv_context.addr_len);
    printf("Received packet from the VM of size: %d.\n", recv_size);
  }
  // protocol loop exited, close stream
  return 0;
}

// main client loop
int main(int32_t argc, char **argv) {
  // unused argv for now -- will use in the actual product
  (void) argv;

  // usage check
  if (argc != 1) {
    printf("Usage: client\n"); // no argument needed
    return -1;
  }

  // initialize the windows socket library if this is a windows client
  #if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
      printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
      return -2;
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
    return -3;
  }
  printf("UDP Send socket created.\n");

  // create receiving UDP socket, this where we will receive from the VM with
  // which we are initiating the protocol
  if ((RECVsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    printf("Unable to create receive socket.\n");
    return -4;
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
    return -5;
  }
  printf("UDP Receive Socket bound to port %d.\n", RECV_PORT);

  // set the hole punching server endpoint to send the first packet to initiate
  // hole punching. We will set another endpoint later for the actual server we
  // will communicate with
  memset(&holepunch_addr, 0, sizeof(holepunch_addr));
  holepunch_addr.sin_family = AF_INET;
  holepunch_addr.sin_port = htons(HOLEPUNCH_PORT);
  holepunch_addr.sin_addr.s_addr = inet_addr(HOLEPUNCH_SERVER_IP);

  // now we need to send a simple datagram to the hole punching server to let it
  // know of our public UDP endpoint. Not only the holepunch server, but the VM
  // will send their data through this endpoint. Since this is a local client,
  // we first send a datagram with our own IPv4 and a tag to let the hole punch
  // server know this is from a local client, and then we will send a second
  // datagram with the IPv4 of the VM we want to be paired with, whic hwe received
  // by authenticating as a user
  char *holepunch_message = get_host_ipv4(); // this host's IPv4
  strcat(holepunch_message, "C"); // add local client tag

  // send our endpoint to the hole punching server
  // NOTE: we send with the RECVsocket so that the hole punch servers maps the port of the RECV socket to
  // then send to it, but will use the SENDsocket to send to the VM after hole punching is done
  if (reliable_udp_sendto(RECVsocket, holepunch_message, strlen(holepunch_message), holepunch_addr, addr_len) < 0) {
    printf("Unable to send client endpoint to hole punching server.\n");
    return -6;
  }
  printf("Local endpoint sent to the hole punching server.\n");

  // sleep two second: IMPORTANT OTHERWISE THE TWO SENDS WILL CLUMP TOGETHER AND
  // THE HOLE PUNCH SERVER WILL FAIL
  Sleep(5000L);

  // now that this is confirmed, since we are a client, we send another message
  // with the IPv4 of the VM we want to be paired with
  char *target_vm_ipv4 = "140.247.148.157"; // azure VM ipv4

  // send the target VM ipv4 to the hole punching server
  // send with RECVsocket here again
  if (reliable_udp_sendto(RECVsocket, target_vm_ipv4, strlen(target_vm_ipv4), holepunch_addr, addr_len) < 0) {
    printf("Unable to send client endpoint to hole punching server.\n");
    return -7;
  }
  printf("Target VM IPv4 sent to the hole punching server.\n");

  // the hole punching server has now mapped our NAT endpoint and "punched" a
  // hole through the NAT for peers to send us direct datagrams, we now look to
  // receive the punched endpoint of the vm we connect with






  while (1) {
    Sleep(5000L);
  }



  // blocking call to wait for the hole punching server to pair this client with
  // the respective VM, last argument is recv timeout
  reliable_udp_recvfrom(RECVsocket, punch_buff, BUFLEN, holepunch_addr, addr_len);
  printf("Received the endpoint of the VM from the hole punch server.\n");

  // now that we received the endpoint, we can copy it to our client struct to
  // recover the endpoint and initiate the protocol
  struct client vm; // struct to hold the endpoint
  memcpy(&vm, punch_buff, sizeof(struct client)); // copy into struct

  // now that we have the memory, we can create the endpoint we send to
  memset(&send_addr, 0, sizeof(send_addr));
  send_addr.sin_family = AF_INET;
  send_addr.sin_port = vm.port; // the port to communicate with, already in byte network order
  send_addr.sin_addr.s_addr = vm.ipv4; // the IP of the vm to send to, already in byte network order




  printf("received port is: %d\n", vm.port);
  printf("received IP is: %d\n", vm.ipv4);
  while (1) {
    Sleep(5000L);
  }







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
