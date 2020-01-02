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
    int i, recv_size, slen = sizeof(context.addr), j = 0, active = 0;
    struct FractalMessage fmsgs[6];
    struct FractalMessage fmsg;

    for(i = 0; i < 60000; i++) {
        if ((recv_size = recvfrom(context.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&context.addr), &slen)) > 0) {
          if(fmsg.type == MESSAGE_KEYBOARD) {
            if(active) {
              fmsgs[j] = fmsg;
              if(fmsg.keyboard.pressed) {
                if(fmsg.keyboard.code != fmsgs[j - 1].keyboard.code) {
                  j++;
                }
              } else {
                ReplayUserInput(fmsgs, j + 1);
                active = 0;
                j = 0;
              }
            } else {
              fmsgs[0] = fmsg;
              if(fmsg.keyboard.pressed && (fmsg.keyboard.code >= 224 && fmsg.keyboard.code <= 231)) {
                active = 1;
                j++;
              } else {
                ReplayUserInput(fmsgs, 1);
              }    
            }
          } else {
            fmsgs[0] = fmsg;
            ReplayUserInput(fmsgs, 1);
          }
        }
    }

    return 0;
}

static int32_t SendVideo(void *opaque) {
    struct SocketContext context = *(struct SocketContext *) opaque;
    int i, slen = sizeof(context.addr);

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
    int bitrate = width * 3000; // estimate bit rate based on output size

    // init encoder
    encoder_t *encoder;
    encoder = create_video_encoder(width, height, CAPTURE_WIDTH, CAPTURE_HEIGHT, bitrate);

    // video variables
    void *capturedframe; // var to hold captured frame, as a void* to RGB pixels
    int sent_size; // var to keep track of size of packets sent

    // while stream is on
    for(i = 0; i < 600000; i++) {
      // capture a frame
      capturedframe = capture_screen(device);

      video_encoder_encode(encoder, capturedframe);

      if (encoder->packet.size != 0) {
        // send packet
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

	audio_capture_device *device;
	device = create_audio_capture_device();

	audio_encoder_t *encoder;
	encoder = create_audio_encoder(96000);

	audio_filter *filter;
	filter = create_audio_filter(device, encoder);

	for(i = 0; i < 600000; i++) {
	  audio_encode_and_send(device, encoder, filter, context);
	}

	/* flush the encoder */
	int got_output;
	avcodec_encode_audio2(encoder->context, &encoder->packet, NULL, &got_output);

	av_frame_free(&encoder->input_frame);
	av_packet_free(&encoder->packet);
	avcodec_free_context(&encoder->context);
	destroy_audio_capture_device(device);
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
        Sleep(3000);
    }
 

    closesocket(InputReceiveContext.s);
    closesocket(VideoContext.s);
    closesocket(AudioContext.s);

    WSACleanup();

    return 0;
}