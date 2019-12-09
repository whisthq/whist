/*
 * This file creates a connection on the client (Windows, MacOS, Linux) device
 * that initiates a connection to their associated server (cloud VM), which then
 * starts streaming its desktop video and audio to the client, while the client
 * starts streaming its user inputs to the server.

 Protocol version: 1.0
 Last modification: 11/30/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#if defined(_WIN32)
  #define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "../include/fractal.h" // header file for protocol functions
#include "../include/webserver.h" // header file for webserver query functions

#include "include/libavcodec/avcodec.h"
#include "include/libavdevice/avdevice.h"
#include "include/libavfilter/avfilter.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/avutil.h"
#include "include/libavfilter/buffersink.h"
#include "include/libavfilter/buffersrc.h"
#include "include/libswscale/swscale.h"
#define SDL_MAIN_HANDLED
#include "include/SDL2/SDL.h"
#include "include/SDL2/SDL_thread.h"

#define WINDOW_W 640
#define WINDOW_H 480

// include Winsock library & disable warning if on Windows client
// include pthread library for unix threads if on linux/macos
#if defined(_WIN32)
  #include <winsock2.h> // lib for socket programming on windows
  #include <windows.h> // general windows lib
  #pragma warning(disable: 4201)
  #pragma warning(disable: 4024) // disable thread warning
  #pragma warning(disable: 4113) // disable thread warning type
  #pragma warning(disable: 4244) // disable u_int to u_short conversion warning
  #pragma warning(disable: 4047) // disable char * indirection levels warning
  #pragma warning(disable: 4477) // printf type warning
  #pragma warning(disable: 4267) // conversion from size_t to int
#else
  #include <pthread.h> // thread library on unix
  #include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// global vars and definitions
#define RECV_BUFFER_LEN 50000 // max len of receive buffer

bool repeat = true; // global flag to stream until disconnection

struct context {
    Uint8 yPlane;
    Uint8 uPlane;
    Uint8 vPlane;
    int uvPitch;
    AVCodecContext* CodecContext;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;
    struct SwsContext* SwsContext;
    SOCKET Socket;
    struct sockaddr_in Address;
};

static AVCodecContext* codecToContext(AVCodec *codec) {
  AVCodecContext* context = avcodec_alloc_context3(codec);
  if (!context) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  context->width = 640;
  context->height = 480;
  context->time_base = (AVRational){1,60};
  // EncodeContext->framerate = (AVRational){30,1};
  context->gop_size = 10;
  context->max_b_frames = 1;
  context->pix_fmt = AV_PIX_FMT_YUV420P;

  av_opt_set(context -> priv_data, "preset", "ultrafast", 0);
  av_opt_set(context -> priv_data, "tune", "zerolatency", 0);

  // and open it
  if (avcodec_open2(context, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }
  return context;
}

// Decode packet into frame
static AVFrame* decode(AVCodecContext *DecodeContext, AVFrame *pFrame, AVPacket packet) {
  int got_output, ret;
  ret = avcodec_decode_video2(DecodeContext, pFrame, &got_output, &packet);
  printf("Decode Status: %d\n", got_output);
  return pFrame;
}

// main thread function to receive server video and audio stream and process it
unsigned __stdcall renderThread(void *opaque) {
  int recv_size;
  AVCodecParserContext* parser;
  struct context* context = (struct context *) opaque;
  AVFrame *pFrame = NULL;
  pFrame = av_frame_alloc();
  // char hexa[] = "0123456789abcdef"; // array of hexadecimal values + null character for deserializing

  // while stream is on, listen for messages
  int sWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
  int sHeight = GetSystemMetrics(SM_CYSCREEN) - 1;


  int addrSize = sizeof(context->Address);
  char buff[100000];

  while(repeat) {

    // set the buffer to empry in case there was previous stuff
    memset(buff, 0, 100000);

    recv_size = recv(context->Socket, buff, 100000, 0);

    printf("test\n");

    if(recv_size == SOCKET_ERROR) {
      printf("Error: %d\n", WSAGetLastError());
    } else {
      printf("size received: %d\n", recv_size);

      // if we received an empty packet, just do nothing
      if (recv_size != 0) {



      // the packet we receive is the FractalMessage struct serialized to hexadecimal,
      // we need to deserialize it to feed it to the Windows API
      // unsigned char fmsg_char[sizeof(AVPacket)]; // array to hold the hexa values in char (decimal) format

      // // first, we need to copy it to a char[] for it to be iterable
      // char iterable_buffer[RECV_BUFFER_LEN] = "";
      // strncpy(iterable_buffer, client_action_buffer, RECV_BUFFER_LEN);

      // // now we iterate over the length of the FractalMessage struct and fill an
      // // array with the decimal value conversion of the hex we received
      // int i, index_0, index_1; // tmp
      // for (i = 0; i < sizeof(AVPacket); i++) {
      //     // find index of the two characters for the current hexadecimal value
      //   index_0 = strchr(hexa, iterable_buffer[i * 2]) - hexa;
      //   index_1 = strchr(hexa, iterable_buffer[(i * 2) + 1]) - hexa;

      //   // now convert back to decimal and store in array
      //   fmsg_char[i] = index_0 * 16 + index_1; // conversion formula
      // }
      // now that we got the de-serialized memory values of the user input, we
      // can copy it back to a FractalMessage struct
      AVPacket packet;
      // av_free_packet(&packet);
      av_init_packet(&packet);
      // memcpy(&packet, &fmsg_char, sizeof(AVPacket));



      // fill in the data back -- need to allocate new pointer here


      // cast the packet data back to its data type
      uint8_t packet_data = (uint8_t) buff;

      // poiner the AVpacket struct poiner to the packet data
      packet.data = &packet_data;


      // set the packet size back in the struct
      packet.size = recv_size;


      pFrame = decode(context->CodecContext, pFrame, packet);
      AVPicture pict;
      pict.data[0] = context->yPlane;
      pict.data[1] = context->uPlane;
      pict.data[2] = context->vPlane;
      pict.linesize[0] = context->CodecContext->width;
      pict.linesize[1] = context->uvPitch;
      pict.linesize[2] = context->uvPitch;

      sws_scale(context->SwsContext, (uint8_t const * const *) pFrame->data,
              pFrame->linesize, 0, context->CodecContext->height, pict.data,
              pict.linesize);

      SDL_UpdateYUVTexture(
              context->Texture,
              NULL,
              context->yPlane,
              context->CodecContext->width,
              context->uPlane,
              context->uvPitch,
              context->vPlane,
              context->uvPitch
          );

      printf("pastabolognese\n");

      SDL_RenderClear(context->Renderer);
      SDL_RenderCopy(context->Renderer, context->Texture, NULL, NULL);
      SDL_RenderPresent(context->Renderer);
    }
  } // closing if packet not empty

  }

  return 0;
}

// main client function
int32_t main(int32_t argc, char **argv) {
  // user login status var
  static bool loggedIn = false;

  // variable for storing user credentials
  char *user_vm_ip = "";

  // usage check for authenticating on our web servers
  if (argc != 3) {
    printf("Usage: client [username] [password]\n");
    return 1;
  }

  // user inputs for username and password
  char *username = argv[1];
  char *password = argv[2];

  // if the user isn't logged in currently, try to authenticate
  if (!loggedIn) {

    // TODO LATER: FIX CODE HERE ON A LOCAL WINDOWS FOR WEBSERVER QUERY
    char *credentials = login(username, password);
    // check if correct authentication
    if (strcmp(credentials, "{}") == 0) {
      // incorrect username or password, couldn't authenticate
      printf("Incorrect username or password.\n");
      // return 2;
    }
    else { // correct credentials, authenticate user
      char* trailing_string = "";
      char* leading_string;
      char* vm_key = "\"public_ip\":\"";
      bool found = false;
      while(*credentials) {
        // printf("%c", *credentials++);
        size_t len = strlen(trailing_string);
        leading_string = malloc(len + 1 + 1 );
        strcpy(leading_string, trailing_string);
        leading_string[len] = *credentials++;
        leading_string[len + 1] = '\0';
        if(found && strstr(leading_string, "\"")) {
          user_vm_ip = trailing_string;
          break;
        }
        trailing_string = leading_string;
        free(leading_string);
        if(strstr(leading_string, vm_key) != NULL) {
          trailing_string = "";
          found = true;
        }
      }
      printf("%s\n", user_vm_ip);
      loggedIn = true; // set user as logged in
    }
    // user authenticated, let's start the protocol
    printf("Successfully authenticated.\n");
  }

  // all good, we have a user and the VM IP written, time to set up the sockets
  // socket environment variables
  SOCKET RECVSocket, SENDSocket; // socket file descriptors
  struct sockaddr_in clientRECV, serverRECV; // this client receive port the server streams to, and the server receive port this client streams to
  int bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
  HANDLE ThreadHandles[1]; // array containing our extra thread handle
  char hexa[17] = "0123456789abcdef"; // array of hexadecimal values + null character for serializing

  // initialize Winsock if this is a Windows client
  #if defined(_WIN32)
    WSADATA wsa;
    // initialize Winsock (sockets library)
    printf("Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
      printf("Failed. Error Code : %d.\n", WSAGetLastError());
      return 3;
    }
    printf("Winsock Initialised.\n");
  #endif

  // Creating our TCP (sending) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // IPPROTO_TCP = TCP protocol
  SENDSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (SENDSocket == INVALID_SOCKET || SENDSocket < 0) { // Windows & Unix cases
    // if can't create socket, return
    printf("Could not create Send TCP socket.\n");
    return 4;
  }
  printf("Send TCP Socket created.\n");

  // prepare the sockaddr_in structure for the send socket (server receive port)
  user_vm_ip = "52.168.122.131"; //aws:"3.90.174.193";


  printf("%d\n", inet_addr(user_vm_ip));
  serverRECV.sin_family = AF_INET; // IPv4
  serverRECV.sin_addr.s_addr = inet_addr(user_vm_ip); // VM (server) IP received from authenticating
  serverRECV.sin_port = htons(config.serverPortRECV); // initial default port 48888

  // connect the client send socket to the server receive port (TCP)
  char *connect_status = connect(SENDSocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV));
  if (connect_status == SOCKET_ERROR || connect_status < 0) {
    printf("Could not connect to the VM (server).\n");
    return 5;
  }
  printf("Connected.\n");

  // now that we're connected, we need to create our receiving UDP socket
  // Creating our UDP (receiving) socket
  // AF_INET = IPv4
  // SOCK_DGAM = UDP Socket
  // IPROTO_UDP = UDP protocol
  RECVSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (RECVSocket == INVALID_SOCKET || RECVSocket < 0) { // Windows & Unix cases
    printf("Could not create Receive UDP socket.\n");
  }
  printf("Receive UDP Socket created.\n");

  // prepare the sockaddr_in structure for the receiving socket
  clientRECV.sin_family = AF_INET; // IPv4
  clientRECV.sin_addr.s_addr = INADDR_ANY; // any IP
  clientRECV.sin_port = htons(config.clientPortRECV); // initial default port 48888

  // for the recv/recvfrom function to work, we need to bind the socket even if it is UDP
  // bind our socket to this port. If it fails, increment port by one and retry
  while (bind(RECVSocket, (struct sockaddr *) &clientRECV, sizeof(clientRECV)) == SOCKET_ERROR) {
    // at most 50 attempts, after that we give up
    if (bind_attempts == 50) {
      printf("Cannot find an open port, abort.\n");
      return 4;
    }
    // display failed attempt
    printf("Bind attempt #%i failed with error code : %d.\n", bind_attempts, WSAGetLastError());

    // increment port number and retry
    bind_attempts += 1;
    clientRECV.sin_port = htons(config.clientPortRECV + bind_attempts); // initial default port 48888
  }
  // successfully binded, we're good to go
  printf("Bind done on port: %d.\n", ntohs(clientRECV.sin_port));

  // since this is a UDP socket, there is no connection necessary
  // now that everything is ready for the communication sockets, we need to init
  // SDL to be ready to process user inputs and display the stream
  // SDL vars for displaying

  AVCodec *H264CodecDecode = avcodec_find_decoder(AV_CODEC_ID_H264);
  AVCodecContext* DecodeContext = codecToContext(H264CodecDecode);
  SDL_Event event;
  SDL_Window *screen;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  Uint8 *yPlane, *uPlane, *vPlane;
  size_t yPlaneSz, uvPlaneSz;
  struct SwsContext *sws_ctx = NULL;
  int got_output,video_stream_idx_cam, videoStream, frameFinished, uvPitch;
  struct context context = {0};

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
      exit(1);
  }

  screen = SDL_CreateWindow(
          "Fractal",
          SDL_WINDOWPOS_UNDEFINED,
          SDL_WINDOWPOS_UNDEFINED,
          DecodeContext->width,
          DecodeContext->height,
          0
      );

  if (!screen) {
      fprintf(stderr, "SDL: could not create window - exiting\n");
      exit(1);
  }

  renderer = SDL_CreateRenderer(screen, -1, 0);
  if (!renderer) {
      fprintf(stderr, "SDL: could not create renderer - exiting\n");
      exit(1);
  }
  // Allocate a place to put our YUV image on that screen
  texture = SDL_CreateTexture(
          renderer,
          SDL_PIXELFORMAT_YV12,
          SDL_TEXTUREACCESS_STREAMING,
          DecodeContext->width,
          DecodeContext->height
      );
  if (!texture) {
      fprintf(stderr, "SDL: could not create texture - exiting\n");
      exit(1);
  }
  // initialize SWS context for software scaling
  sws_ctx = sws_getContext(DecodeContext->width, DecodeContext->height,
          DecodeContext->pix_fmt, DecodeContext->width, DecodeContext->height,
          AV_PIX_FMT_YUV420P,
          SWS_BILINEAR,
          NULL,
          NULL,
          NULL);

  // set up YV12 pixel array (12 bits per pixel)
  yPlaneSz = DecodeContext->width * DecodeContext->height;
  uvPlaneSz = DecodeContext->width * DecodeContext->height / 4;
  yPlane = (Uint8*)malloc(yPlaneSz);
  uPlane = (Uint8*)malloc(uvPlaneSz);
  vPlane = (Uint8*)malloc(uvPlaneSz);
  if (!yPlane || !uPlane || !vPlane) {
      fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
      exit(1);
  }

  uvPitch = DecodeContext->width / 2;

  // TODO LATER: function to call to adapt window size to client

  context.yPlane = yPlane;
  context.uPlane = uPlane;
  context.vPlane = vPlane;
  context.uvPitch = uvPitch;
  context.CodecContext = DecodeContext;
  context.Renderer = renderer;
  context.Texture = texture;
  context.SwsContext = sws_ctx;
  context.Socket = RECVSocket;
  context.Address = serverRECV;
  SDL_Thread *render_thread = SDL_CreateThread(renderThread, "renderThread", &context);
//   clock_t start, end;
  // double cpu_time_used;

  // loop indefinitely to keep sending to the server until repeat set to fasl
  SDL_Event msg;
  while (repeat) {
    // poll for an SDL event
    if (SDL_PollEvent(&msg)) {

  //    start = clock();


      // event received, define Fractal message and find which event type it is
      FractalMessage fmsg = {0};

      switch (msg.type) {
        // SDL event for keyboard key pressed or released
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          // fill Fractal message structure for sending
          fmsg.type = MESSAGE_KEYBOARD;
          fmsg.keyboard.code = (FractalKeycode) msg.key.keysym.scancode;
          fmsg.keyboard.mod = msg.key.keysym.mod;
          fmsg.keyboard.pressed = msg.key.type == SDL_KEYDOWN; // print statement to see what's happening
          break;
        // SDL event for mouse location when it moves
        case SDL_MOUSEMOTION:
          fmsg.type = MESSAGE_MOUSE_MOTION;
          // fmsg.mouseMotion.relative = SDL_GetRelativeMouseMode();
          // fmsg.mouseMotion.x = fmsg.mouseMotion.relative ? msg.motion.xrel : msg.motion.x;
          // fmsg.mouseMotion.y = fmsg.mouseMotion.relative ? msg.motion.yrel : msg.motion.y;
          fmsg.mouseMotion.x = msg.motion.xrel;
          fmsg.mouseMotion.y = msg.motion.yrel;
          // printf("Mouse Position: (%d, %d)\n", fmsg.mouseMotion.x, fmsg.mouseMotion.y); // print statement to see what's happening
          break;
        // SDL event for mouse button pressed or released
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          fmsg.type = MESSAGE_MOUSE_BUTTON;
          fmsg.mouseButton.button = msg.button.button;
          fmsg.mouseButton.pressed = msg.button.type == SDL_MOUSEBUTTONDOWN;
          // printf("Mouse Button Code: %d\n", fmsg.mouseButton.button); // print statement to see what's happening
          break;
        // SDL event for mouse wheel scroll
        case SDL_MOUSEWHEEL:
          fmsg.type = MESSAGE_MOUSE_WHEEL;
          fmsg.mouseWheel.x = msg.wheel.x;
          fmsg.mouseWheel.y = msg.wheel.y;
          // printf("Mouse Scroll Position: (%d, %d)\n", fmsg.mouseWheel.x, fmsg.mouseWheel.y); // print statement to see what's happening
          break;
        case SDL_QUIT:
          repeat = false;
          SDL_DestroyTexture(texture);
          SDL_DestroyRenderer(renderer);
          SDL_DestroyWindow(screen);
          SDL_Quit();
          exit(0);
          break;
        // TODO LATER: clipboard switch case
      }
      // we broke out of the listen loop, so we have an event
      // if we have a message type identified, send event to server
      if (fmsg.type != 0) {
        // fmsg is a FractalMessage struct, so we need to serialzie it (convert
        // to string) to send it over the network
        // we first copy the memory bits to a char array
        unsigned char fmsg_char[sizeof(struct FractalMessage)];
        memcpy(fmsg_char, &fmsg, sizeof(struct FractalMessage));
        char fmsg_serialized[2 * sizeof(struct FractalMessage) + 1]; // serialized array is 2x the length since hexa

        // loop over the char struct, convert each value to hexadecimal
        int i;
        for (i = 0; i < sizeof(struct FractalMessage); i++) {
          // converting to hexa
          fmsg_serialized[i * 2] = hexa[fmsg_char[i] / 16];
          fmsg_serialized[(i * 2 ) + 1] = hexa[fmsg_char[i] % 16];
        }
        // printf("User action serialized, ready for sending.\n");


  //      end = clock();
    //    cpu_time_used = ((double) (end - start)); /// CLOCKS_PER_SEC;
      //  printf("user action packet took %f seconds to execute \n", cpu_time_used);


        // user input is serialized, ready to stream over the network
        // send data message to server
        if (send(SENDSocket, fmsg_serialized, strlen(fmsg_serialized), 0) < 0) {
          // error sending, terminate
          printf("Send failed with error code: %d\n", WSAGetLastError());
          return 7;
        }
        // printf("User action sent.\n");
      }
    }
  }
  // left the while loop, protocol received instruction to terminate
  printf("Connection interrupted.\n");

  // TODO LATER: Split this file into Windows/Unix and do threads for Unix for max efficiency


  // Windows case, closing sockets
  #if defined(_WIN32)
    // threads are done, let's close the extra handle and exit
    CloseHandle(ThreadHandles[0]);

    // close the sockets
    closesocket(RECVSocket);
    closesocket(SENDSocket);
    WSACleanup(); // close Windows socket library
  #else
    close(RECVSocket);
    close(SENDSocket);
  #endif

  // write close time to server, set loggedin to false and return
  //logout(username);
  //loggedIn = false; // logout user
  return 0;
}

#ifdef __cplusplus
}
#endif

// re-enable Windows warning, if Windows client
#if defined(_WIN32)
  #pragma warning(default: 4201)
  #pragma warning(default: 4024)
  #pragma warning(default: 4113)
  #pragma warning(default: 4244)
  #pragma warning(default: 4047)
  #pragma warning(default: 4477)
  #pragma warning(default: 4267)
#endif
