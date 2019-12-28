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
    int recv_size, slen = sizeof(context.addr);
    char recv_buf[BUFLEN];
    char *message = "Keyboard input\n";

    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0) {
            printf("Could not send packet\n");
        }
    }
}

static int32_t SendStream2(void *opaque) {
    struct context context = *(struct context *) opaque;
    struct sockaddr_in receive_address;
    int recv_size, slen = sizeof(receive_address);
    char recv_buf[BUFLEN];
    char *message = "ACK1";

    while(1) {
        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&receive_address), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), sizeof(context.addr)) < 0)
                printf("Could not send packet\n");
        }
    }
}

static int32_t SendStream3(void *opaque) {
    struct context context = *(struct context *) opaque;
    struct sockaddr_in receive_address;
    int recv_size, slen = sizeof(receive_address);
    char recv_buf[BUFLEN];
    char *message = "ACK2";

    while(1) {
        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&receive_address), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), sizeof(context.addr)) < 0)
                printf("Could not send packet\n");
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

    struct sockaddr_in a;
    int recv_size, slen = sizeof(a);
    char recv_buf[BUFLEN];

    struct context UserInputContext = {0};
    if(CreateUDPContext(&UserInputContext, "C", "40.117.57.45", -1) < 0) {
        exit(1);
    }

    struct context VideoAckContext = {0};
    if(CreateUDPContext(&VideoAckContext, "C", "40.117.57.45", -1) < 0) {
        exit(1);
    }

    struct context AudioAckContext = {0};
    if(CreateUDPContext(&AudioAckContext, "C", "40.117.57.45", -1) < 0) {
        exit(1);
    }

    SDL_Thread *send_stream_1 = SDL_CreateThread(SendStream1, "SendStream1", &UserInputContext);
    // SDL_Thread *send_stream_2 = SDL_CreateThread(SendStream2, "SendStream2", &VideoAckContext);
    // SDL_Thread *send_stream_3 = SDL_CreateThread(SendStream3, "SendStream3", &VideoAckContext);
 
    while(1) {
        if ((recv_size = recvfrom(UserInputContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&UserInputContext.addr), &slen)) < 0) {
            printf("Packet not received \n");
        }
    }
    // Actually, we never reach this point...
    closesocket(UserInputContext.s);
    // closesocket(VideoAckContext.s);

    WSACleanup();

    return 0;
}