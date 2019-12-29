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

#define BUFLEN 160000


static int32_t SendUserInput(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Keyboard Input";

    for(i = 0; i < 50; i++) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0) {
            printf("Could not send packet\n");
        } else {
            printf("Sent keyboard info\n");
        }
        Sleep(1000);
    }

    return 0;
}

static int32_t ReceiveVideo(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    int recv_size;
    char recv_buf[BUFLEN];

    while (1)
    {
        printf("Listneing for video\n");
        if ((recv_size = recvfrom(context.s, &recv_buf, BUFLEN, 0, (struct sockaddr*)(&context.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received video\n");
        }
    }
}

static int32_t ReceiveAudio(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    int recv_size;
    char recv_buf[BUFLEN];

    while (1)
    {
        printf("Listneing for audio\n");
        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received audio\n");
        }
    }

    return 0;
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
    if(CreateUDPContext(&InputContext, "C", "3.232.55.216", -1, 20) < 0) {
        exit(1);
    }

    struct context VideoReceiveContext = {0};
    if(CreateUDPContext(&VideoReceiveContext, "C", "3.232.55.216", -1, 0) < 0) {
        exit(1);
    }

    struct context AudioReceiveContext = {0};
    if(CreateUDPContext(&AudioReceiveContext, "C", "3.232.55.216", -1, 0) < 0) {
        exit(1);
    }

    SDL_Thread *send_input = SDL_CreateThread(SendUserInput, "SendUserInput", &InputContext);
    SDL_Thread *receive_video = SDL_CreateThread(ReceiveVideo, "ReceiveVideo", &VideoReceiveContext);
    SDL_Thread *receive_audio = SDL_CreateThread(ReceiveAudio, "ReceiveAudio", &AudioReceiveContext);

    while (1)
    {
        if (SendAck(&VideoReceiveContext.s) < 0)
            printf("Could not send packet\n");
        if (SendAck(&AudioReceiveContext.s) < 0)
            printf("Could not send packet\n");
        if ((recv_size = recvfrom(InputContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&InputContext.addr), &slen)) < 0)
            printf("Packet not received \n");
        // Sleep(2000);
    }
 
    // Actually, we never reach this point...

    closesocket(InputContext.s);
    closesocket(VideoReceiveContext.s);

    WSACleanup();

    return 0;
}