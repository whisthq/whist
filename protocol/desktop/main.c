/*
 * This file creates a connection on the client (Windows, MacOS, Linux) device
 * that initiates a connection to their associated server (cloud VM), which then
 * starts streaming its desktop video and audio to the client, while the client
 * starts streaming its user inputs to the server.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël, Ming Ying

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

#include "../include/fractal.h" // header file for protocol functions
#include "../include/webserver.h" // header file for webserver query functions
#include "../include/decode.h" // header file for decoder

#define SDL_MAIN_HANDLED // SDL definition
#include "../include/SDL2/SDL.h"
#include "../include/SDL2/SDL_thread.h"

// placeholder window size, will get actual monitor size
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
  #include <unistd.h>
#endif

// global vars and definitions
#define RECV_BUFFER_LEN 160000 // max len of receive buffer
bool repeat = true; // global flag to stream until disconnection


struct context {
    Uint8 *yPlane;
    Uint8 *uPlane;
    Uint8 *vPlane;
    int uvPitch;
    SDL_Renderer* Renderer;
    SDL_Texture* Texture;
    SOCKET Socket;
    struct SwsContext* sws;
};



typedef enum {
	videotype = 0xFA010000,
	audiotype = 0xFB010000
} Fractalframe_type_t;

typedef struct {
	Fractalframe_type_t type;
	int size;
	char data[0];
} Fractalframe_t;

#define FRAME_BUFFER_SIZE (1920 * 1080)




// main thread function to receive server video and audio stream and process it
static int32_t renderThread(void *opaque) {
  // cast socket and SDL variables back to their data type for usage
  struct context* context = (struct context *) opaque;

  // arbitrary values for testing for now
  int height = 1920;
  int width = 1080;
  int bitrate = width * 10000; // estimate bit rate based on output size

  // init decoder
  decoder_t *decoder;
  decoder = create_decoder(width, height, width, height, bitrate);

  // init receiving variables
  int recv_size; // var to keep track of size of packets received
  char buff[RECV_BUFFER_LEN]; // buffer to receive the packets





  // initdecoded frame parameters
  Fractalframe_t *decodedframe = (Fractalframe_t *) malloc(FRAME_BUFFER_SIZE);
  memset(decodedframe, 0, FRAME_BUFFER_SIZE); // set memory to null
  decodedframe->type = videotype; // specify that this is a video frame

  // while stream is on
  while (repeat) {
    // query for packets reception indefinitely via recv until repeat set to false
    recv_size = recv(context->Socket, buff, RECV_BUFFER_LEN, 0);
    printf("Received packet of size: %d\n", recv_size);

    // if the packet isn't empty (aka there is an action to process
    if (recv_size > 0) {
      // decode the packet we received into a frame
      decoder_decode(decoder, buff, recv_size, decodedframe->data);

      AVPicture pict;
      pict.data[0] = context->yPlane;
      pict.data[1] = context->uPlane;
      pict.data[2] = context->vPlane;
      pict.linesize[0] = 1920;
      pict.linesize[1] = context->uvPitch;
      pict.linesize[2] = context->uvPitch;
         printf("decoder context height %d\n", decoder->context->height);
         sws_scale(context->sws, (uint8_t const * const *) decoder->frame->data,
                 decoder->frame->linesize, 0, decoder->context->height, pict.data,
                 pict.linesize);

          SDL_UpdateYUVTexture(
                  context->Texture,
                  NULL,
                  context->yPlane,
                  1920,
                  context->uPlane,
                  context->uvPitch,
                  context->vPlane,
                  context->uvPitch
              );
          SDL_RenderClear(context->Renderer);
          SDL_RenderCopy(context->Renderer, context->Texture, NULL, NULL);
          SDL_RenderPresent(context->Renderer);
    }
    // frame displayed, let's reset the decoded frame memory for the next one
    memset(decodedframe, 0, FRAME_BUFFER_SIZE);
  }
  // exited while loop, stream done let's close everything
  destroy_decoder(decoder); // destroy encoder
  return 0;
}




// main client function
int32_t main(int32_t argc, char **argv) {


  // user login status var
  static bool loggedIn = false;


  SDL_Event msg;
  SDL_Window *screen;
  SDL_Renderer *renderer;
  SDL_Texture *texture;

  Uint8 *yPlane, *uPlane, *vPlane;
  size_t yPlaneSz, uvPlaneSz;
  int uvPitch;
  struct context context = {0};

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

    // query the webservers
    char *credentials = login(username, password);

    // check if correct authentication
    if (strcmp(credentials, "{}") == 0) {
      // incorrect username or password, couldn't authenticate
      printf("Incorrect username or password.\n");
      return 2;
    }

    // easy
    user_vm_ip = parse_response(credentials);
    printf("%s\n", user_vm_ip);
    loggedIn = true; // set user as logged in

    // user authenticated, let's start the protocol
    printf("Successfully authenticated.\n");
  }

  // all good, we have a user and the VM IP written, time to set up the sockets
  // socket environment variables
  SOCKET RECVSocket, SENDSocket; // socket file descriptors
  struct sockaddr_in clientRECV, serverRECV; // this client receive port the server streams to, and the server receive port this client streams to
  int bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
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
  user_vm_ip = /*"52.168.122.131";*/"140.247.148.157"; // aws one


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
  int timeout = 1000;
  int sizeTimeout = sizeof(int);
  setsockopt(RECVSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeTimeout);
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

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
      exit(1);
  }

  screen = SDL_CreateWindow(
          "Fractal",
          SDL_WINDOWPOS_UNDEFINED,
          SDL_WINDOWPOS_UNDEFINED,

          1920, // width
          1080, // height
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
          1920, // width
          1080 // height
      );
  if (!texture) {
      fprintf(stderr, "SDL: could not create texture - exiting\n");
      exit(1);
  }

  struct SwsContext *sws_ctx = NULL;
  sws_ctx = sws_getContext(1920, 1080,
          AV_PIX_FMT_YUV420P, 1920, 1080,
          AV_PIX_FMT_YUV420P,
          SWS_BILINEAR,
          NULL,
          NULL,
          NULL);

  // set up YV12 pixel array (12 bits per pixel)
  yPlaneSz = 1920 * 1080;
  uvPlaneSz = 1920 * 1080 / 4;
  yPlane = (Uint8*)malloc(yPlaneSz);
  uPlane = (Uint8*)malloc(uvPlaneSz);
  vPlane = (Uint8*)malloc(uvPlaneSz);
  if (!yPlane || !uPlane || !vPlane) {
      fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
      exit(1);
  }

  uvPitch = 1920 / 2;

  // TODO LATER: function to call to adapt window size to client

  context.yPlane = yPlane;
  context.sws = sws_ctx;
  context.uPlane = uPlane;
  context.vPlane = vPlane;
  context.uvPitch = uvPitch;
  context.Renderer = renderer;
  context.Texture = texture;
  context.Socket = RECVSocket;


  SDL_Thread *render_thread = SDL_CreateThread(renderThread, "renderThread", &context);



  // loop indefinitely to keep sending to the server until repeat set to fasl
  while (repeat) {
    // poll for an SDL event
    if (SDL_PollEvent(&msg)) {


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


  // free al that shit
  free(yPlane);
  free(uPlane);
  free(vPlane);



  // Windows case, closing sockets
  #if defined(_WIN32)
    // threads are done, let's close the extra handle and exit

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
