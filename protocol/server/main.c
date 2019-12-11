/*
 * This file creates a server on the host (Windows 10) VM that waits for a
 * connection request from a client, and then starts streaming its desktop video
 * and audio to the client, and receiving the user input back.

 Protocol version: 1.0
 Last modification: 12/10/2019

 By: Philippe Noël, Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <winuser.h>
#include <ws2tcpip.h> // other Windows socket library
#include <winsock2.h> // lib for socket programming on windows
#include <process.h> // for threads programming

#include "../include/fractal.h" // header file for protocol functions

// ffmpeg libraries
#include "include/libavcodec/avcodec.h"
#include "include/libavdevice/avdevice.h"
#include "include/libavfilter/avfilter.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/avutil.h"
#include "include/libavfilter/buffersink.h"
#include "include/libavfilter/buffersrc.h"

// ffmpeg codec definition
#define CODEC_FLAG_GLOBAL_HEADER AV_CODEC_FLAG_GLOBAL_HEADER
#define CODEC_FLAG_LOOP_FILTER AV_CODEC_FLAG_LOOP_FILTER

// Winsock Library
#pragma comment(lib, "ws2_32.lib")

#if defined(_WIN32)
  #pragma warning(disable: 4201)
  #pragma warning(disable: 4024) // disable thread warning
  #pragma warning(disable: 4113) // disable thread warning type
  #pragma warning(disable: 4244) // disable u_int to u_short conversion warning
  #pragma warning(disable: 4047) // disable char * indirection levels warning
  #pragma warning(disable: 4701) // potentially unitialized var
  #pragma warning(disable: 4477) // printf type warning
  #pragma warning(disable: 4267) // conversion from size_t to int
  #pragma warning(disable: 4996) // strncpy unsafe warning
#endif

#ifdef __cplusplus
extern "C" {
#endif

// global vars and definitions
bool repeat = true; // global flag to keep streaming until client disconnects
// client sends packets of len 33, this fitted size prevents packets clumping
#define RECV_BUFFER_LEN 33

struct SocketContext {
  SOCKET Socket;
  struct sockaddr_in Address;
};

// Create an AVCodecContext object for a given codec
static AVCodecContext* codecToContext(AVCodec *codec) {
  AVCodecContext* context = avcodec_alloc_context3(codec);
  if (!context) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  context->width = 640;
  context->height = 480;
  context->time_base = (AVRational){1,30};
  // EncodeContext->framerate = (AVRational){30,1};
  context->gop_size = 10;
  context->max_b_frames = 1;
  context->pix_fmt = AV_PIX_FMT_YUV420P;
  // context->bit_rate = 1000000;
  av_opt_set(context -> priv_data, "preset", "ultrafast", 0);
  av_opt_set(context -> priv_data, "tune", "zerolatency", 0);

  // and open it
  if (avcodec_open2(context, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }
  return context;
}

// main function to stream the video and audio from this server to the client
unsigned __stdcall SendStream(void *opaque) {
  // Initialize variables/functions
  av_register_all();
  avcodec_register_all();
  avdevice_register_all();
  avfilter_register_all();

  // Create codecs, contexts, packets, dictionaries, etc.
  AVCodec *pCodecInCam;
  AVCodecContext *ScreenCaptureContext = NULL;
  AVFrame *cam_frame, *filt_frame;
  AVPacket packet;
  AVFormatContext *pFormatCtx = NULL;
  AVFrame *pFrame = NULL;
  AVCodec *H264CodecEncode = avcodec_find_encoder(AV_CODEC_ID_H264);
  AVCodec *H264CodecDecode = avcodec_find_decoder(AV_CODEC_ID_H264);
  AVDictionary *inOptions = NULL;
  AVFormatContext *pFormatCtxInCam;


  char args_cam[512];
  int i, ret, got_output,video_stream_idx_cam;

  // Initialize codec contexts
  AVCodecContext* EncodeContext = codecToContext(H264CodecEncode);
  AVCodecContext* DecodeContext = codecToContext(H264CodecDecode);

  // Initialize variables used for screen recording
  avcodec_parameters_alloc();
  AVInputFormat *inFrmt= av_find_input_format("dshow");
  AVFilter *buffersrc_cam  = avfilter_get_by_name("buffer");
  AVFilter *buffersink = avfilter_get_by_name("buffersink");
  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs  = avfilter_inout_alloc();
  AVFilterContext *buffersink_ctx;
  AVFilterContext *buffersrc_ctx_cam;
  AVFilterGraph *filter_graph;
  AVBufferSinkParams *buffersink_params;
  pFormatCtxInCam = avformat_alloc_context();
  cam_frame = av_frame_alloc();
  filt_frame = av_frame_alloc();
  video_stream_idx_cam = -1;
  size_t sent_size;

  // Set screen recording resolution and frame rate
  // av_dict_set(&inOptions, "video_size", "1280x720", 0);
  av_dict_set(&inOptions, "frame_rate", "30", 0);

  // Specify screen capture device and open decoders
  ret = avformat_open_input(&pFormatCtxInCam, "video=screen-capture-recorder", inFrmt, &inOptions);
  if(avformat_find_stream_info(pFormatCtxInCam,NULL)<0)
    return -1;

  for(i=0; i < pFormatCtxInCam->nb_streams; i++)
    if(pFormatCtxInCam->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
      video_stream_idx_cam=i;

  if(video_stream_idx_cam == -1)
    return -1;

  ScreenCaptureContext = pFormatCtxInCam->streams[video_stream_idx_cam]->codec;
  pCodecInCam = avcodec_find_decoder(ScreenCaptureContext->codec_id);

  if(pCodecInCam == NULL){
    fprintf(stderr,"decoder not found");
    exit(1);
  }

  if (avcodec_open2(ScreenCaptureContext, pCodecInCam, NULL) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder for webcam\n");
    exit(1);
  }

  if (!cam_frame || !filt_frame) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }

  enum AVPixelFormat pix_fmts[] = { ScreenCaptureContext->pix_fmt,  AV_PIX_FMT_NONE};
  filter_graph = avfilter_graph_alloc();

  snprintf(args_cam, sizeof(args_cam),
      "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      ScreenCaptureContext->width, ScreenCaptureContext->height, ScreenCaptureContext->pix_fmt,
      ScreenCaptureContext->time_base.num, ScreenCaptureContext->time_base.den,
      ScreenCaptureContext->sample_aspect_ratio.num, ScreenCaptureContext->sample_aspect_ratio.den);

  ret = avfilter_graph_create_filter(&buffersrc_ctx_cam, buffersrc_cam, "in",
      args_cam, NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
    exit(ret);
  }

  buffersink_params = av_buffersink_params_alloc();
  buffersink_params->pixel_fmts = pix_fmts;

  ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
      NULL, buffersink_params, filter_graph);
  av_free(buffersink_params);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
    exit(ret);
  }

  outputs->name       = av_strdup("in");
  outputs->filter_ctx = buffersrc_ctx_cam;
  outputs->pad_idx    = 0;
  outputs->next       = NULL;

  inputs->name       = av_strdup("out");
  inputs->filter_ctx = buffersink_ctx;
  inputs->pad_idx    = 0;
  inputs->next       = NULL;


  const char *filter_str = "scale='640:480',format='yuv420p'";
  if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_str,
          &inputs, &outputs, NULL)) < 0){
    printf("error in graph parse");
    exit(ret);
  }

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0){
    printf("error in graph config");
    exit(ret);
  }

  // Open codec
  if (avcodec_open2(DecodeContext, H264CodecDecode, NULL) < 0)
      return -1; // Could not open codec

  while(repeat) {
    av_init_packet(&packet);

    ret = av_read_frame(pFormatCtxInCam,&packet);

    if(ret < 0){
      fprintf(stderr,"Error reading frame\n");
      break;
    }

    // chaeck if its a avideo frame
    if(packet.stream_index == video_stream_idx_cam){
      packet.dts = av_rescale_q_rnd(packet.dts,
          pFormatCtxInCam->streams[video_stream_idx_cam]->time_base,
          pFormatCtxInCam->streams[video_stream_idx_cam]->codec->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
      packet.pts = av_rescale_q_rnd(packet.pts,
          pFormatCtxInCam->streams[video_stream_idx_cam]->time_base,
          pFormatCtxInCam->streams[video_stream_idx_cam]->codec->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      ret = avcodec_decode_video2(ScreenCaptureContext,cam_frame,&got_output,&packet);

      cam_frame->pts = av_frame_get_best_effort_timestamp(cam_frame);

      if (got_output) {

        av_free_packet(&packet);
        av_init_packet(&packet);

        // add frame to filter graph
        ret = av_buffersrc_add_frame_flags(buffersrc_ctx_cam, cam_frame, 0);

        // get the frames from the filter graph
        while(1){
          filt_frame = av_frame_alloc();
          if ( ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
            break;
          }

          // we'll get a frame that we can now encode to
          ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
          if (ret < 0) {
            // if there are no more frames or end of frames that is normal
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
              ret = 0;
            av_frame_free(&filt_frame);
            break;
          }

          ret = avcodec_encode_video2(EncodeContext, &packet, filt_frame, &got_output);
          struct SocketContext* sendContext = (struct SocketContext*) opaque;

          // only send if the packet is not empty obviously
          if (packet.size != 0) {
            if ((sent_size = send(sendContext->Socket, packet.data, packet.size, 0)) < 0) {
              printf("Socket sending error \n");
            }
            av_free_packet(&packet);
            av_frame_free(&filt_frame);
          }
        }
      }
    }
    av_free_packet(&packet);
  }

  // flush out delayed frames
  for (got_output = 1; got_output; i++) {
    ret = avcodec_encode_video2(EncodeContext, &packet, NULL, &got_output);
    if (ret < 0) {
      fprintf(stderr, "Error encoding frame\n");
      exit(1);
    }
    if (got_output) {
      av_free_packet(&packet);
    }
  }


  // Free the YUV frame
  av_frame_free(&pFrame);
  // Close the codec
  avcodec_close(DecodeContext);
  avcodec_close(EncodeContext);

  // Close the video file
  avformat_close_input(&pFormatCtx);
  _endthreadex(0);
  return 0;
}




// main function to receive client user inputs and process them
unsigned __stdcall ReceiveClientInput(void *RECVsocket_param) {
  // cast the socket parameter back to socket for use
  SOCKET RECVsocket = *(SOCKET *) RECVsocket_param;

  // initiate buffer to store the user action that we will receive
  int recv_size;
  char *client_action_buffer[RECV_BUFFER_LEN];
  char hexa[] = "0123456789abcdef"; // array of hexadecimal values + null character for deserializing

  // while stream is on, listen for messages
  int sWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
  int sHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
  while (repeat) {
    // query for packets reception indefinitely via recv until repeat set to false
    recv_size = recv(RECVsocket, client_action_buffer, RECV_BUFFER_LEN, 0);
    //printf("Received packet of size: %d\n", recv_size);
    //printf("Message received: %s\n", client_action_buffer);

    // if the packet isn't empty (aka there is an action to process)
    if (recv_size != 0) {
      // the packet we receive is the FractalMessage struct serialized to hexadecimal,
      // we need to deserialize it to feed it to the Windows API
      unsigned char fmsg_char[sizeof(struct FractalMessage)]; // array to hold the hexa values in char (decimal) format

      // first, we need to copy it to a char[] for it to be iterable
      char iterable_buffer[RECV_BUFFER_LEN] = "";
      strncpy(iterable_buffer, client_action_buffer, RECV_BUFFER_LEN);

      // now we iterate over the length of the FractalMessage struct and fill an
      // array with the decimal value conversion of the hex we received
      int i, index_0, index_1; // tmp
      for (i = 0; i < sizeof(struct FractalMessage); i++) {
        // find index of the two characters for the current hexadecimal value
      index_0 = strchr(hexa, iterable_buffer[i * 2]) - hexa;
      index_1 = strchr(hexa, iterable_buffer[(i * 2) + 1]) - hexa;

      // now convert back to decimal and store in array
      fmsg_char[i] = index_0 * 16 + index_1; // conversion formula
      }
      // now that we got the de-serialized memory values of the user input, we
      // can copy it back to a FractalMessage struct
      FractalMessage fmsg = {0};
      memcpy(&fmsg, &fmsg_char, sizeof(struct FractalMessage));

      // all good, we're back with our user input to replay
      //printf("User action deserialized, ready for replaying.\n");

      // time to create an input event for the windows API based on our Fractal event
      INPUT Event = {0};

      // switch to fill in the Windows event depending on the FractalMessage type
      switch (fmsg.type) {
        // Windows event for keyboard action
        case MESSAGE_KEYBOARD:
          Event.ki.wVk = virtual_codes[fmsg.keyboard.code - 4];
          Event.type = INPUT_KEYBOARD;
          Event.ki.wScan = 0;
          Event.ki.time = 0; // system supplies timestamp

          if (!fmsg.keyboard.pressed) {
          Event.ki.dwFlags = KEYEVENTF_KEYUP;
          }

          break;
        case MESSAGE_MOUSE_MOTION:
          Event.type = INPUT_MOUSE;
          Event.mi.dx = fmsg.mouseMotion.x * ((float)65536 / sWidth);
          Event.mi.dy = fmsg.mouseMotion.y * ((float)65536 / sHeight);
          Event.mi.dwFlags = MOUSEEVENTF_MOVE;


          break;
        case  MESSAGE_MOUSE_BUTTON:
          Event.type = INPUT_MOUSE;
          Event.mi.dx = 0;
          Event.mi.dy = 0;
          if (fmsg.mouseButton.button == 1) {
            if (fmsg.mouseButton.pressed) {
              Event.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            }
            else {
              Event.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            }
          } else if (fmsg.mouseButton.button == 3) {
            if (fmsg.mouseButton.pressed) {
              Event.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            }
            else {
              Event.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            }
          }
          break;
        case MESSAGE_MOUSE_WHEEL:
          Event.type = INPUT_MOUSE;
          Event.mi.dwFlags = MOUSEEVENTF_WHEEL;
          Event.mi.dx = 0;
          Event.mi.dy = 0;
          Event.mi.mouseData = fmsg.mouseWheel.x;
          break;
      }
      // now that we have mapped our FMSG to a Windows event, let's play it
      SendInput(1, &Event, sizeof(INPUT)); // 1 structure to send
    }
  }
  // connection interrupted by setting repeat to false, exit protocol
  printf("Connection interrupted.\n");

  // terminate thread as we are done with the stream
  _endthreadex(0);
  return 0;
}

// main server function
int32_t main(int32_t argc, char **argv) {
   // unused argv
   (void) argv;

  // usage check
  if (argc != 1) {
    printf("Usage: server\n"); // no argument needed, server listens
    return 1;
  }

  // socket environment variables
  WSADATA wsa;
  SOCKET listensocket, RECVsocket, SENDsocket; // socket file descriptors
  struct sockaddr_in serverRECV, clientRECV, clientServerRECV; // file descriptors for the ports, clientRECV = clientServerRECV
  int clientServerRECV_addr_len, bind_attempts = 0;
  FractalConfig config = FRACTAL_DEFAULTS; // default port settings
  HANDLE ThreadHandles[2]; // array containing our 2 thread handles

  // initialize Winsock (sockets library)
  printf("Initialising Winsock...\n");
  if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
    printf("Failed. Error Code : %d.\n", WSAGetLastError());
    return 2;
  }
  printf("Winsock Initialised.\n");

  // Creating our TCP (receiving) socket (need it first to initiate connection)
  // AF_INET = IPv4
  // SOCK_STREAM = TCP Socket
  // 0 = protocol automatically detected
  if ((listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
    printf("Could not create listen TCP socket: %d.\n" , WSAGetLastError());
    WSACleanup(); // close Windows socket library
    return 3;
  }
  printf("Listen TCP Socket created.\n");

  // prepare the sockaddr_in structure for the listening socket
  serverRECV.sin_family = AF_INET; // IPv4
  serverRECV.sin_addr.s_addr = INADDR_ANY; // any IP
  serverRECV.sin_port = htons(config.serverPortRECV); // initial default port

  // bind our socket to this port. If it fails, increment port by one and retry
  while (bind(listensocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV)) == SOCKET_ERROR) {
    // at most 50 attempts, after that we give up
    if (bind_attempts == 50) {
      printf("Cannot find an open port, abort.\n");
      closesocket(listensocket); // close open socket
      WSACleanup(); // close Windows socket library
      return 4;
    }
    // display failed attempt
    printf("Bind attempt #%i failed with error code: %d.\n", bind_attempts, WSAGetLastError());

    // increment port number and retry
    bind_attempts += 1;
    serverRECV.sin_port = htons(config.serverPortRECV + bind_attempts); // initial default port 48888
  }
  // successfully binded, we're good to go
  printf("Bind done on port: %d.\n", ntohs(serverRECV.sin_port));

  // this passive socket is always open to listen for an incoming connection
  listen(listensocket, 1); // 1 maximum concurrent connection
  printf("Waiting for an incoming connection...\n");

  // forever loop to always be listening to pick up a new connection if idle
  while (true) {
    // accept client connection once there's one on the listensocket queue
    // new active socket created on same port as listensocket,
    clientServerRECV_addr_len = sizeof(struct sockaddr_in);
    RECVsocket = accept(listensocket, (struct sockaddr *) &clientServerRECV, &clientServerRECV_addr_len);
    if (RECVsocket == INVALID_SOCKET) {
      // print error but keep listening to new potential connections
      printf("Accept failed with error code: %d.\n", WSAGetLastError());
    }
    else {
      // now that we got our receive socket ready to receive client input, we
      // need to create our send socket to initiate the stream
      printf("Connection accepted - Receive TCP Socket created.\n");

      // Creating our UDP (sending) socket
      // AF_INET = IPv4
      // SOCK_DGRAM = UDP Socket
      // IPPROTO_UDP = UDP protocol
      if ((SENDsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        printf("Could not create UDP socket : %d.\n" , WSAGetLastError());
        closesocket(listensocket); // close open socket
        closesocket(RECVsocket); // close open socket
        WSACleanup(); // close Windows socket library
        return 5;
      }
      printf("Send UDP Socket created.\n");

      // to bind to the client receiving port, we need its IP and port, which we
      // can get since we have accepted connection on our receiving port
      char *client_ip = inet_ntoa(clientServerRECV.sin_addr);

      // prepare the sockaddr_in structure for the sending socket
      clientRECV.sin_family = AF_INET; // IPv4
      clientRECV.sin_addr.s_addr = inet_addr(client_ip); // client IP to send stream to
      clientRECV.sin_port = htons(config.clientPortRECV); // client port to send stream to

      // since this is a UDP socket, there is no connection necessary
      // however, we do a connect to force only this specific server IP onto the
      // client socket to ensure our stream will stay intact
      // connect the server send socket to the client receive port (UDP)
      char *connect_status = connect(SENDsocket, (struct sockaddr *) &clientRECV, sizeof(clientRECV));
      if (connect_status == SOCKET_ERROR) {
        printf("Could not connect to the client w/ error: %d\n", WSAGetLastError());
        closesocket(listensocket); // close open socket
        closesocket(RECVsocket); // close open socket
        closesocket(SENDsocket); // close open socket
        WSACleanup(); // close Windows socket library
        return 3;
      }
      printf("Connected on port: %d.\n", config.clientPortRECV);

      // time to start streaming and receiving user input
      // launch thread #1 to start streaming video & audio from server
      struct SocketContext sendContext = {0};
      sendContext.Socket = SENDsocket;
      sendContext.Address = clientRECV;

      ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &SendStream, &sendContext, 0, NULL);

      // launch thread #2 to start receiving and processing client input
      ThreadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, &ReceiveClientInput, &RECVsocket, 0, NULL);

      // TODO LATER: Add a third thread that listens for disconnect and sets repeat=false

      // block until our 2 threads terminate, so until the protocol terminates
      WaitForMultipleObjects(2, ThreadHandles, true, INFINITE);

      // threads are done, let's close their handles and exit
      CloseHandle(ThreadHandles[0]);
      CloseHandle(ThreadHandles[1]);
    }
    // client disconnected, restart listening for connections
    printf("Client disconnected.\n");

    // client disconnected, close the send socket since it's client specific
    // threads already closed from within their respective function
    closesocket(SENDsocket);
  }
  // server loop exited, close everything
  closesocket(RECVsocket);
  closesocket(listensocket);
  WSACleanup(); // close Windows socket library
  return 0;
}

#ifdef __cplusplus
}
#endif

// re-enable Windows warnings
#if defined(_WIN32)
  #pragma warning(default: 4201)
  #pragma warning(default: 4024)
  #pragma warning(default: 4113)
  #pragma warning(default: 4244)
  #pragma warning(default: 4047)
  #pragma warning(default: 4701)
  #pragma warning(default: 4477)
  #pragma warning(default: 4267)
  #pragma warning(default: 4996)
#endif
