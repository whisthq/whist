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


static int32_t SendUserInput(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Keyboard Input";

    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0)
            printf("Could not send packet\n");
    }
}

static int32_t ReceiveVideo(void *opaque) {
    printf("1");
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    int recv_size;
    char recv_buf[BUFLEN];

    while (1)
    {
        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received %s\n", recv_buf);
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

    struct context InputContext = {0};
    if(CreateUDPContext(&InputContext, "C", "40.117.57.45", -1, 0) < 0) {
        exit(1);
    }

    struct context VideoReceiveContext = {0};
    if(CreateUDPContext(&VideoReceiveContext, "C", "40.117.57.45", -1, 0) < 0) {
        exit(1);
    }

    SDL_Thread *send_input = SDL_CreateThread(SendUserInput, "SendUserInput", &InputContext);
    SDL_Thread *receive_video = SDL_CreateThread(ReceiveVideo, "ReceiveVideo", &VideoReceiveContext);
    // SDL_Thread *send_stream_2 = SDL_CreateThread(SendStream2, "SendStream2", &ReceiveContext);

    // memset((char *) &receive_address, 0, sizeof(receive_address));
    // receive_address.sin_family = AF_INET;
    // receive_address.sin_port = htons(PORT + 1);
    // receive_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // struct context VideoContext = {0};
    // VideoContext.addr = receive_address;
    // VideoContext.s = UserInputAckContext.s;

    char* ack1 = "ACK";
    while (1)
    {
        if ((recv_size = recvfrom(InputContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&InputContext.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received %s\n", recv_buf);
        }
        if (sendto(VideoReceiveContext.s, ack1, strlen(ack1), 0, (struct sockaddr*)(&VideoReceiveContext.addr), slen) < 0)
            printf("Could not send packet\n");
        // Sleep(2000);
    }
 
    // Actually, we never reach this point...

    closesocket(InputContext.s);
    closesocket(VideoReceiveContext.s);

    WSACleanup();

    return 0;
}