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

static int32_t ReceiveUserInput(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, recv_size, slen = sizeof(context.addr);
    char recv_buf[BUFLEN];
    char *message = "ACK";

    while(1) {
        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context.addr), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received %s\n", recv_buf);
        }
    }
}

static int32_t SendVideo(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Video";

    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0)
            printf("Could not send packet\n");
    }
}

static int32_t SendAudio(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Audio";

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

    struct sockaddr_in receive_address;
    int recv_size, slen=sizeof(receive_address);
    char recv_buf[BUFLEN];

    struct context InputAckContext = {0};
    if(CreateUDPContext(&InputAckContext, "S", "", -1, 0) < 0) {
        exit(1);
    }

    struct context VideoContext = {0};
    if(CreateUDPContext(&VideoContext, "S", "", -1, 20) < 0) {
        exit(1);
    }

    struct context AudioContext = {0};
    if(CreateUDPContext(&AudioContext, "S", "", -1, 20) < 0) {
        exit(1);
    }


    SDL_Thread *send_input_ack = SDL_CreateThread(ReceiveUserInput, "ReceiveUserInput", &InputAckContext);
    SDL_Thread *send_video = SDL_CreateThread(SendVideo, "SendVideo", &VideoContext);
    SDL_Thread *send_audio = SDL_CreateThread(SendAudio, "SendAudio", &AudioContext);

    char *message = "ACK";
    while (1)
    {
        if (sendto(InputAckContext.s, message, strlen(message), 0, (struct sockaddr*)(&InputAckContext.addr), slen) < 0)
            printf("Could not send packet\n");
        if ((recv_size = recvfrom(VideoContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&VideoContext.addr), &slen)) < 0) 
            printf("Packet not received \n");
        if ((recv_size = recvfrom(AudioContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&AudioContext.addr), &slen)) < 0) 
            printf("Packet not received \n");
    }
 
    // Actually, we never reach this point...

    closesocket(InputAckContext.s);
    closesocket(VideoContext.s);

    WSACleanup();

    return 0;
}