// UDP hole punching example, client code
// Base UDP code stolen from http://www.abc.se/~m6695/udp.html
// By Oscar Rodriguez
// This code is public domain, but you're a complete lunatic
// if you plan to use this code in any real program.
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/fractal.h"

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 1000
 
static int32_t SendStream1(void *opaque) {
    struct context context = *(struct context *) opaque;
    int slen = sizeof(context.addr);

    // Once again, the payload is irrelevant. Feel free to send your VoIP
    // data in here.
    char *message = "Hello from the first stream!";
    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0)
            printf("Could not send packet\n");
    }
}

static int32_t SendStream2(void *opaque) {
    struct context context = *(struct context *) opaque;
    int slen = sizeof(context.addr);

    // Once again, the payload is irrelevant. Feel free to send your VoIP
    // data in here.
    char *message = "Hello from the second stream!";
    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0)
            printf("Could not send packet\n");
    }
}


int main(int argc, char* argv[])
{
    // initialize the windows socket library if this is a windows client
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }
    printf("Winsock initialized successfully.\n");

    struct sockaddr_in receive_address, send_address;
    int recv_size, slen=sizeof(send_address);
    char recv_buf[BUFLEN];
    SOCKET s;
 
    memset((char *) &receive_address, 0, sizeof(receive_address));
    receive_address.sin_family = AF_INET;
    receive_address.sin_port = htons(PORT + 1);
    receive_address.sin_addr.s_addr = htonl(INADDR_ANY);

    struct context SendContext = {0};
    SendContext.s = s;
    SendContext.addr = send_address;

    if(CreateUDPSendContext(&SendContext, "C", "40.117.57.45", -1) < 0) {
        exit(1);
    }

    struct context ReceiveContext = {0};
    ReceiveContext.s = SendContext.s;
    ReceiveContext.addr = receive_address;

    SDL_Thread *send_stream_1 = SDL_CreateThread(SendStream1, "SendStream1", &SendContext);
    SDL_Thread *send_stream_2 = SDL_CreateThread(SendStream2, "SendStream2", &SendContext);

    while (1)
    {
        if ((recv_size = recvfrom(ReceiveContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&ReceiveContext.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received message: %s\n", recv_buf);
        }
    }

 
    // Actually, we never reach this point...
    closesocket(SendContext.s);
    closesocket(ReceiveContext.s);

    WSACleanup();

    return 0;
}