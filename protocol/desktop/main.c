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


#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

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
//extern "C" {
#endif

// global vars and definitions
#define RECV_BUFFER_LEN 5000 // max len of receive buffer

struct SDL_Context {
  bool done;
  SDL_Window *window;
  SDL_Surface *surface;
  SDL_Cursor *cursor;
  SOCKET RECVSocket;
  SDL_AudioDeviceID audio;
};
bool repeat = true; // global flag to stream until disconnection

// main thread function to receive server video and audio stream and process it
static int32_t renderThread(struct SDL_Context SDL_Context) {	
  // initiate buffer to store the reply
   SDL_GLContext *gl = SDL_GL_CreateContext(SDL_Context.window);
   SDL_GL_SetSwapInterval(1);
  while (repeat) {
    int recv_size;
    char *server_reply[RECV_BUFFER_LEN];
    AVCodec *codec;
    AVCodecContext *context= NULL;
    int frame_count;
    AVFrame *frame;
    AVFormatContext *ifmt_ctx = NULL;
    int len, got_frame, ret;
    AVCodecParserContext *pCodecParserCtx=NULL;

    av_register_all();
    avformat_network_init();

    // while stream is on, listen for messages
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
     if (!codec) {
         printf("Codec not found\n");
         exit(1);
     }
     context = avcodec_alloc_context3(codec);
     if (!context) {
         printf("Could not allocate video codec context\n");
         exit(1);
     }
     // if(codec->capabilities&CODEC_CAP_TRUNCATED)
     //     c->flags|= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

    pCodecParserCtx=av_parser_init(AV_CODEC_ID_H264);
    if (!pCodecParserCtx){
      printf("Could not allocate video parser context\n");
      return -1;
    }

     if (avcodec_open2(context, codec, NULL) < 0) {
         printf("Could not open codec\n");
         exit(1);
     }
     frame = av_frame_alloc();
     if (!frame) {
         printf("Could not allocate video frame\n");
         exit(1);
     }
    // query for packets reception indefinitely via recv until repeat set to false
    recv_size = recv(SDL_Context.RECVSocket, server_reply, RECV_BUFFER_LEN, 0);
    printf("bytes received: %d, server reply size: %d\n", recv_size, sizeof(server_reply));
    AVPacket packet;
    av_init_packet(&packet);

    uint8_t udp_array[sizeof(server_reply)];
    packet.data = udp_array;
    memcpy(packet.data, &server_reply, sizeof(server_reply));
    packet.size = sizeof(server_reply);
    unsigned char in_buffer[4096 + 100]={0};
    unsigned char *cur_ptr;
    cur_ptr=in_buffer;
    len = av_parser_parse2(
      pCodecParserCtx, context,
      &packet.data, &packet.size,
      cur_ptr, 4096, 
      AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
    ret = avcodec_send_packet(context, &packet);
    while(ret >= 0) {
      ret = avcodec_receive_frame(context, frame);
      // if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      //   printf("frame error");
      // } else {
      //   printf("frame received");
      // }
    }
    // ret = avcodec_decode_video2(context, frame, &got_frame, &packet);
    printf("%d\n", got_frame);
    if(got_frame + 1) {
      printf("got frame!");
    }
  }
  // connection interrupted by setting repeat to false, exit protocol
  printf("Connection interrupted.\n");

	// terminate thread as we are done with the stream
	_endthreadex(0);
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
  SOCKET RECVSocket, SENDsocket; // socket file descriptors
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
  SENDsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (SENDsocket == INVALID_SOCKET || SENDsocket < 0) { // Windows & Unix cases
    // if can't create socket, return
    printf("Could not create Send TCP socket.\n");
    return 4;
  }
  printf("Send TCP Socket created.\n");

  // prepare the sockaddr_in structure for the send socket (server receive port)
  user_vm_ip = "140.247.148.129"; //aws:"3.90.174.193";


  printf("%d\n", inet_addr(user_vm_ip));
	serverRECV.sin_family = AF_INET; // IPv4
  serverRECV.sin_addr.s_addr = inet_addr(user_vm_ip); // VM (server) IP received from authenticating
  serverRECV.sin_port = htons(config.serverPortRECV); // initial default port 48888

	// connect the client send socket to the server receive port (TCP)
	char *connect_status = connect(SENDsocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV));
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
  SDL_Surface *imageSurface = NULL;
  SDL_Surface *windowSurface = NULL;
  SDL_Window *window = NULL;

  // init SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
      printf("SDL init error.\n");
      return 5;
  }
  printf("SDL initialized.\n");

  // TODO LATER: function to call to adapt window size to client

  // set the SDL window properties
  struct SDL_Context context = {0};
  context.window = SDL_CreateWindow("Fractal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_W, WINDOW_H, SDL_WINDOW_OPENGL);
  int32_t glW = 0;
  SDL_GL_GetDrawableSize(context.window, &glW, NULL);

  SDL_Renderer* sdlRenderer;
  SDL_Texture* sdlTexture;
  SDL_Rect sdlRect;

  sdlRenderer = SDL_CreateRenderer(context.window, -1, 0); 
  sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,WINDOW_W,WINDOW_H); 
  
  sdlRect.x=0;
  sdlRect.y=0;
  sdlRect.w=WINDOW_W;
  sdlRect.h=WINDOW_H;

  // windowSurface = SDL_GetWindowSurface(context.window);
  // if (window == NULL) {
  //     printf("SDL window error.\n");
  //     return 6;
  // }

  // now that SDL is ready, time to start the protocol
  // the user inputs sending is done in thread 1 (this thread)  while the video
  // receiving from the server is done in a second thread
  // launch thread #2 to start streaming video & audio from server
  // ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveStream, &RECVsocket, 0, NULL);

  // all good there, time to start sending user input
  // define SDL variable to listen for user inputs
  SDL_Event msg;
  context.RECVSocket = RECVSocket;
  SDL_Thread *render_thread = SDL_CreateThread(renderThread, "renderThread", &context);

//   clock_t start, end;
  // double cpu_time_used;

  // loop indefinitely to keep sending to the server until repeat set to fasl
  while (repeat) {
    // poll for an SDL event
    if (SDL_PollEvent(&msg)) {

  //    start = clock();


      // event received, define Fractal message and find which event type it is
      FractalMessage fmsg = {0};

      switch (msg.type) {
        // SDL quit event, exit switch
        case SDL_QUIT:
          repeat = false; // used clicked to close the window, close protocol
          break;
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
        if (send(SENDsocket, fmsg_serialized, strlen(fmsg_serialized), 0) < 0) {
          // error sending, terminate
          printf("Send failed with error code: %d\n", WSAGetLastError());
          return 7;
        }
        // printf("User action sent.\n");
      }
    }
    // packet sent, let's update the SDL surface
    SDL_BlitSurface(imageSurface, NULL, windowSurface, NULL);
    SDL_UpdateWindowSurface(window);
  }
  // left the while loop, protocol received instruction to terminate
  printf("Connection interrupted.\n");

  // TODO LATER: Split this file into Windows/Unix and do threads for Unix for max efficiency

  // client or server disconnected, close everything
  // free SDL and set variables to null
  SDL_FreeSurface(imageSurface);
  SDL_FreeSurface(windowSurface);

  // terminate SDL
  SDL_DestroyWindow(window);
  SDL_Quit();

  // Windows case, closing sockets
  #if defined(_WIN32)
    // threads are done, let's close the extra handle and exit
    CloseHandle(ThreadHandles[0]);

    // close the sockets
    closesocket(RECVSocket);
    closesocket(SENDsocket);
    WSACleanup(); // close Windows socket library
  #else
    close(RECVSocket);
    close(SENDsocket);
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
