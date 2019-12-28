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
    struct sockaddr_in receive_address;
    int recv_size, slen = sizeof(receive_address);
    char recv_buf[BUFLEN];
    char *message = "ACK1";

    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), sizeof(context.addr)) < 0)
            printf("Could not send packet\n");
    }
}

static int32_t SendStream2(void *opaque) {
    struct context context = *(struct context *) opaque;
    struct sockaddr_in receive_address;
    int recv_size, slen = sizeof(context.addr);
    char recv_buf[BUFLEN];
    char *message = "Video\n";

    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0) {
            printf("Could not send packet\n");
        } else {
            if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&receive_address), &slen)) < 0) {
                printf("Packet not received \n");
            } else {
                printf("Received %s\n", recv_buf);
            }
        }
    }
}

static int32_t SendStream3(void *opaque) {
    struct context context = *(struct context *) opaque;
    struct sockaddr_in receive_address;
    int recv_size, slen = sizeof(context.addr);
    char recv_buf[BUFLEN];
    char *message = "Audio\n";

    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0) {
            printf("Could not send packet\n");
        } else {
            if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&receive_address), &slen)) < 0) {
                printf("Packet not received \n");
            } else {
                printf("Received %s\n", recv_buf);
            }
        }
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

    struct sockaddr_in receive_address;
    int recv_size, slen=sizeof(receive_address);
    char recv_buf[BUFLEN];

    struct context UserInputAckContext = {0};
    if(CreateUDPContext(&UserInputAckContext, "S", "", -1) < 0) {
        exit(1);
    }

    struct context VideoContext = {0};
    if(CreateUDPContext(&VideoContext, "S", "", -1) < 0) {
        exit(1);
    }

    struct context AudioContext = {0};
    if(CreateUDPContext(&AudioContext, "S", "", -1) < 0) {
        exit(1);
    }

    SDL_Thread *send_stream_1 = SDL_CreateThread(SendStream1, "SendStream1", &UserInputAckContext);
    // SDL_Thread *send_stream_2 = SDL_CreateThread(SendStream2, "SendStream2", &VideoContext);
    // SDL_Thread *send_stream_3 = SDL_CreateThread(SendStream3, "SendStream3", &AudioContext);

    // memset((char *) &receive_address, 0, sizeof(receive_address));
    // receive_address.sin_family = AF_INET;
    // receive_address.sin_port = htons(PORT + 1);
    // receive_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // struct context VideoContext = {0};
    // VideoContext.addr = receive_address;
    // VideoContext.s = UserInputAckContext.s;

    while(1) {
        if ((recv_size = recvfrom(UserInputAckContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&UserInputAckContext.addr), &slen)) < 0) {
            printf("Packet not received \n");
        }
    }
 
    // Actually, we never reach this point...
    closesocket(UserInputAckContext.s);
    // closesocket(VideoContext.s);

    WSACleanup();

    return 0;
}