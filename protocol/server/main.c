#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/fractal.h"
#include "../include/videocapture.h"
#include "../include/audiocapture.h"
#include "../include/videoencode.h"
#include "../include/audioencode.h"

#define SDL_MAIN_HANDLED
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 1000
#define RECV_BUFFER_LEN 38 // exact user input packet line to prevent clumping
#define FRAME_BUFFER_SIZE (1024 * 1024)
#define MAX_PACKET_SIZE 1000

bool repeat = true; // global flag to keep streaming until client disconnects

typedef enum {
  videotype = 0xFA010000,
  audiotype = 0xFB010000
} Fractalframe_type_t;

typedef struct {
  Fractalframe_type_t type;
  int size;
  char data[0];
} Fractalframe_t;


static int SendPacket(struct SocketContext *context, uint8_t *data, int len) {
  int sent_size, payload_size, slen = sizeof(context->addr), i = 0;
  uint8_t payload[MAX_PACKET_SIZE];
  int curr_index = 0;

  while (curr_index < len) {
    payload_size = min((MAX_PACKET_SIZE), (len - curr_index));
    if((sent_size = sendto(context->s, (data + curr_index), payload_size, 0, (struct sockaddr*)(&context->addr), slen)) < 0) {
      return -1;
    } else {
      curr_index += payload_size;
    }
  }
  return 0;
}

static int32_t ReceiveUserInput(void *opaque) {
    struct SocketContext context = *(struct SocketContext *) opaque;
    int i, recv_size, slen = sizeof(context.addr);
    char recv_buf[BUFLEN];

    for(i = 0; i < 3000; i++) {
        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context.addr), &slen)) < 0) {
            printf("Packet not received \n");
        }
    }

    return 0;
}

static int32_t SendVideo(void *opaque) {
    struct SocketContext context = *(struct SocketContext *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Video";

    // get window
    HWND window = NULL;
    window = GetDesktopWindow();
    frame_area frame = {0, 0, 0, 0}; // init  frame area

    // init screen capture device
    video_capture_device *device;
    device = create_video_capture_device(window, frame);

    // set encoder parameters
    int width = device->width; // in and out from the capture device
    int height = device->height; // in and out from the capture device
    int bitrate = width * 8000; // estimate bit rate based on output size

    // init encoder
    encoder_t *encoder;
    encoder = create_video_encoder(width, height, CAPTURE_WIDTH, CAPTURE_HEIGHT, bitrate);

    // video variables
    void *capturedframe; // var to hold captured frame, as a void* to RGB pixels
    int sent_size; // var to keep track of size of packets sent

    // while stream is on
    for(i = 0; i < 3000; i++) {
      // capture a frame
      capturedframe = capture_screen(device);

      video_encoder_encode(encoder, capturedframe);

      if (encoder->packet.size != 0) {
        // send packet
        // if (SendPacket(&context, "MHYVIDOEISPLAYINGRIGHTNOWSOIAMTESTINGATSTARBUKCS", strlen("MHYVIDOEISPLAYINGRIGHTNOWSOIAMTESTINGATSTARBUKCS")) < 0) {
        // printf("Sending length %d\n", encoder->packet.size);
        if (SendPacket(&context, encoder->packet.data, encoder->packet.size) < 0) {
            printf("Could not send video frame\n");
        }
      }
  }

  // exited while loop, stream done let's close everything
  destroy_video_encoder(encoder); // destroy encoder
  destroy_video_capture_device(device); // destroy capture device
  return 0;
}

static int32_t SendAudio(void *opaque) {
    struct SocketContext context = *(struct SocketContext *) opaque;
    int i, slen = sizeof(context.addr);
    char *message = "Audio";

    for(i = 0; i < 3000; i++) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.addr), slen) < 0)
            printf("Could not send packet\n");
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

    struct sockaddr_in receive_address;
    int recv_size, slen=sizeof(receive_address);
    char recv_buf[BUFLEN];

    struct SocketContext InputReceiveContext = {0};
    if(CreateUDPContext(&InputReceiveContext, "S", "", -1) < 0) {
        exit(1);
    }

    struct SocketContext VideoContext = {0};
    if(CreateUDPContext(&VideoContext, "S", "", 100) < 0) {
        exit(1);
    }

    struct SocketContext AudioContext = {0};
    if(CreateUDPContext(&AudioContext, "S", "", 100) < 0) {
        exit(1);
    }

    SDL_Thread *send_input_ack = SDL_CreateThread(ReceiveUserInput, "ReceiveUserInput", &InputReceiveContext);
    SDL_Thread *send_video = SDL_CreateThread(SendVideo, "SendVideo", &VideoContext);
    SDL_Thread *send_audio = SDL_CreateThread(SendAudio, "SendAudio", &AudioContext);

    while (1)
    {
        if (SendAck(&InputReceiveContext) < 0)
            printf("Could not send packet\n");
        if ((recv_size = recvfrom(VideoContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&VideoContext.addr), &slen)) < 0) 
            printf("Packet not received \n");
        if ((recv_size = recvfrom(AudioContext.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&AudioContext.addr), &slen)) < 0) 
            printf("Packet not received \n");
        Sleep(2000);
    }
 
    // Actually, we never reach this point...

    closesocket(InputReceiveContext.s);
    closesocket(VideoContext.s);
    closesocket(AudioContext.s);

    WSACleanup();

    return 0;
}