/*
 * This file creates a server on the host (Windows 10) VM that waits for a
 * connection request from a client, and then starts streaming its desktop video
 * and audio to the client, and receiving the user input back.

 Protocol version: 1.0
 Last modification: 11/30/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // silence the deprecated warnings

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <Winuser.h>
#include <ws2tcpip.h> // other Windows socket library (of course, thanks #Microsoft)
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
#define RECV_BUFFER_LEN 33 // our protocol sends packets of len 33, this prevents two packets clumping together in the socket buffer
bool repeat = true; // global flag to keep streaming until client disconnects
char virtual_codes[140] =
{ 0x41,
0x42,
0x43,
0x44,
0x45,
0x46,
0x47,
0x48,
0x49,
0x4A,
0x4B,
0x4C,
0x4D,
0x4E,
0x4F,
0x50,
0x51,
0x52,
0x53,
0x54,
0x55,
0x56,
0x57,
0x58,
0x59,
0x5A,
0x31,
0x32,
0x33,
0x34,
0x35,
0x36,
0x37,
0x38,
0x39,
0x30,
0x0D,
0x1B,
0x08,
0x09,
0x20,
0xBD,
0xBB,
0xDB,
0xDD,
0xDC,
0xBA,
0xDE,
0xC0,
0xBC,
0xBE,
0xBF,
0x14,
0x70,
0x71,
0x72,
0x73,
0x74,
0x75,
0x76,
0x77,
0x78,
0x79,
0x7A,
0x7B,
0x2C,
0x91,
0x13,
0x2D,
0x24,
0x21,
0x2E,
0x23,
0x22,
0x27,
0x25,
0x28,
0x26,
0x90,
0x6F,
0x6A,
0x6D,
0x6B,
0x0D,
0x61,
0x62,
0x63,
0x64,
0x65,
0x66,
0x67,
0x68,
0x69,
0x60,
0xBE,
0x5D,
0x7C,
0x7D,
0x7E,
0x7F,
0x80,
0x81,
0x82,
0xA4,
0xAF,
0xAE,
0xA2,
0xA0,
0x12,
0x5B,
0xA3,
0x12,
0xA3,
0xB0,
0xB1,
0xB2,
0xB3,
0xAD,
0xB5,
0x7FFFFFFF };

// main function to stream the video and audio from this server to the client
unsigned __stdcall SendStream(void *SENDsocket_param) {
	// cast the socket parameter back to socket for use
	SOCKET SENDsocket = *(SOCKET *) SENDsocket_param;




 // Register all codecs, devices, filters with the library
  av_register_all();
  avcodec_register_all();
  avdevice_register_all();
  avfilter_register_all();

  // Codecs
  AVCodec *pCodecOut;
  AVCodec *pCodecInCam;

  // In and out
  AVCodecContext *pCodecCtxOut= NULL;
  AVCodecContext *pCodecCtxInCam = NULL;

  char args_cam[512]; // we'll use this in the filter_graph

  int i, ret, got_output,video_stream_idx_cam;

  // the frames to be used
  AVFrame *cam_frame,*outFrame,*filt_frame;
  AVPacket packet;

  // output filename
  const char *filename = "out.h264";

  AVOutputFormat *pOfmtOut = NULL;
  AVStream *strmVdoOut = NULL;

  // we will capture from a dshow device
  // @see https://trac.ffmpeg.org/wiki/DirectShow
  // @see http://www.ffmpeg.org/ffmpeg-devices.html#dshow
  AVInputFormat *inFrmt= av_find_input_format("dshow");

  // used to set the input options
  AVDictionary *inOptions = NULL;

  // set the format context
  AVFormatContext *pFormatCtxOut;
  AVFormatContext *pFormatCtxInCam;

  // to create the filter graph
  AVFilter *buffersrc_cam  = avfilter_get_by_name("buffer");
  AVFilter *buffersink = avfilter_get_by_name("buffersink");

  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs  = avfilter_inout_alloc();

  AVFilterContext *buffersink_ctx;
  AVFilterContext *buffersrc_ctx_cam;
  AVFilterGraph *filter_graph;

  AVBufferSinkParams *buffersink_params;

  // the filtergraph, string, scale keeping the height constant, and setting the pixel format
  const char *filter_str = "scale='w=-1:h=480',format='yuv420p'"; // add padding for 16:9 and then scale to 1280x720

  /////////////////////////// decoder

  // create the new context
  pFormatCtxInCam = avformat_alloc_context();


	printf("test1\n"); // run

  // set input resolution
  // av_dict_set(&inOptions, "video_size", "1280x720", 0);
  av_dict_set(&inOptions, "frame_rate", "30", 0);

	printf("test2\n"); // run

  // input device, since we selected dshow
  ret = avformat_open_input(&pFormatCtxInCam, "video=screen-capture-recorder", inFrmt, &inOptions);

	printf("test3\n");

  // lookup infor
  if(avformat_find_stream_info(pFormatCtxInCam,NULL)<0)
    return -1;

  video_stream_idx_cam = -1;

  // find the stream index, we'll only be encoding the video for now
  for(i=0; i<pFormatCtxInCam->nb_streams; i++)
    if(pFormatCtxInCam->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
      video_stream_idx_cam=i;

  if(video_stream_idx_cam == -1)
    return -1;

  // set the coded ctx & codec
  pCodecCtxInCam = pFormatCtxInCam->streams[video_stream_idx_cam]->codec;

  pCodecInCam = avcodec_find_decoder(pCodecCtxInCam->codec_id);

  if(pCodecInCam == NULL){
    fprintf(stderr,"decoder not found");
    exit(1);
  }

  // open it
  if (avcodec_open2(pCodecCtxInCam, pCodecInCam, NULL) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder for webcam\n");
    exit(1);
  }

  //////////////////////// encoder

  // find the format based on the filename
  pOfmtOut = av_guess_format(NULL, filename,NULL);

  pFormatCtxOut = avformat_alloc_context();

  // set the codec
  pOfmtOut->video_codec = AV_CODEC_ID_H264;

  // set the output format
  pFormatCtxOut->oformat = pOfmtOut;

  snprintf(pFormatCtxOut->filename,sizeof(pFormatCtxOut->filename),"%s",filename);

  // add a new video stream
  strmVdoOut = avformat_new_stream(pFormatCtxOut,NULL);
  if (!strmVdoOut ) {
    fprintf(stderr, "Could not alloc stream\n");
    exit(1);
  }

  pCodecCtxOut = strmVdoOut->codec;
  if (!pCodecCtxOut) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  // set the output codec params
  pCodecCtxOut->codec_id = pOfmtOut->video_codec;
  pCodecCtxOut->codec_type = AVMEDIA_TYPE_VIDEO;

  // pCodecCtxOut->bit_rate = 1200000;
  pCodecCtxOut->pix_fmt = AV_PIX_FMT_YUV420P;
  pCodecCtxOut->width = 640;
  pCodecCtxOut->height = 480;
  pCodecCtxOut->time_base = (AVRational){1,30};
  // pCodecCtxOut->preset = "slow";


  pCodecCtxOut->bit_rate = 500*1000;
  pCodecCtxOut->bit_rate_tolerance = 0;
  pCodecCtxOut->rc_max_rate = 0;
  pCodecCtxOut->rc_buffer_size = 0;
  pCodecCtxOut->gop_size = 40;
  pCodecCtxOut->max_b_frames = 3;
  pCodecCtxOut->b_frame_strategy = 1;
  pCodecCtxOut->coder_type = 1;
  pCodecCtxOut->me_cmp = 1;
  pCodecCtxOut->me_range = 16;
  pCodecCtxOut->qmin = 10;
  pCodecCtxOut->qmax = 51;
  pCodecCtxOut->scenechange_threshold = 40;
  pCodecCtxOut->flags |= CODEC_FLAG_LOOP_FILTER;
  // pCodecCtxOut->me_method = ME_HEX;
  pCodecCtxOut->me_subpel_quality = 5;
  pCodecCtxOut->i_quant_factor = 0.71;
  pCodecCtxOut->qcompress = 0.6;
  pCodecCtxOut->max_qdiff = 4;
  // pCodecCtxOut->directpred = 1;
  pCodecCtxOut->flags2 |= AV_CODEC_FLAG2_FAST;

  // if the format wants global header, we'll set one
  if(pFormatCtxOut->oformat->flags & AVFMT_GLOBALHEADER)
    pCodecCtxOut->flags |= CODEC_FLAG_GLOBAL_HEADER;

  // find the encoder
  pCodecOut = avcodec_find_encoder(pCodecCtxOut->codec_id);
  //avcodec_find_encoder_by_name("h264_nvenc");
  if (!pCodecOut) {
    fprintf(stderr, "Codec not found\n");
    exit(1);
  }

  // and open it
  if (avcodec_open2(pCodecCtxOut, pCodecOut,NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  // open the file for writing
  if (avio_open(&pFormatCtxOut->pb, filename, AVIO_FLAG_WRITE) <0) {
    fprintf(stderr, "Could not open '%s'\n", filename);
    exit(1);
  }

  // write the format headers
  ret = avformat_write_header(pFormatCtxOut, NULL);
  if(ret < 0 ){
    fprintf(stderr, "Could not write header '%d'\n", ret);
    exit(1);
  }

  ////////////////// create frame

  // allocate frames
  // these can be done down under also
  cam_frame = av_frame_alloc();
  outFrame = av_frame_alloc();
  filt_frame = av_frame_alloc();

  if (!cam_frame || !outFrame || !filt_frame) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }

  ////////////////////////// fix pix fmt
  enum AVPixelFormat pix_fmts[] = { pCodecCtxInCam->pix_fmt,  AV_PIX_FMT_NONE};

  /////////////////////////// FILTER GRAPH

  // create a filter graph
  filter_graph = avfilter_graph_alloc();

  snprintf(args_cam, sizeof(args_cam),
      "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      pCodecCtxInCam->width, pCodecCtxInCam->height, pCodecCtxInCam->pix_fmt,
      pCodecCtxInCam->time_base.num, pCodecCtxInCam->time_base.den,
      pCodecCtxInCam->sample_aspect_ratio.num, pCodecCtxInCam->sample_aspect_ratio.den);

  // input source buffer
  ret = avfilter_graph_create_filter(&buffersrc_ctx_cam, buffersrc_cam, "in",
      args_cam, NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
    exit(ret);
  }

  buffersink_params = av_buffersink_params_alloc();
  buffersink_params->pixel_fmts = pix_fmts;

  // output sink
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

  // parse the filter string we set
  if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_str,
          &inputs, &outputs, NULL)) < 0){
    printf("error in graph parse");
    exit(ret);
  }

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0){
    printf("error in graph config");
    exit(ret);
  }
  //////////////////////////////DUMP

  // for info stuff
  av_dump_format(pFormatCtxInCam,0,"screen-capture-recorder",0);
  av_dump_format(pFormatCtxOut,0,filename,1);

  ////////////////////////// TRANSCODE

  // since a dshow is never ending, i've just a static number
  for(i=0;i<300;i++){
    av_init_packet(&packet);

    // get a frame from input
    ret = av_read_frame(pFormatCtxInCam,&packet);

    if(ret < 0){
      fprintf(stderr,"Error reading frame\n");
      break;
    }

    // chaeck if its a avideo frame
    if(packet.stream_index == video_stream_idx_cam){

      // set the dts & pts
      // @see http://stackoverflow.com/q/6044330/651547
      packet.dts = av_rescale_q_rnd(packet.dts,
          pFormatCtxInCam->streams[video_stream_idx_cam]->time_base,
          pFormatCtxInCam->streams[video_stream_idx_cam]->codec->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
      packet.pts = av_rescale_q_rnd(packet.pts,
          pFormatCtxInCam->streams[video_stream_idx_cam]->time_base,
          pFormatCtxInCam->streams[video_stream_idx_cam]->codec->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      // decode from the format and store in cam_frame
      ret = avcodec_decode_video2(pCodecCtxInCam,cam_frame,&got_output,&packet);

      cam_frame->pts = av_frame_get_best_effort_timestamp(cam_frame);

      if (got_output) {

        av_free_packet(&packet);
        av_init_packet(&packet);

        // add frame to filter graph
        ret = av_buffersrc_add_frame_flags(buffersrc_ctx_cam, cam_frame, 0);




        // get the frames from the filter graph
        while(repeat){
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

          // this is redundant
          outFrame = filt_frame;

          // encode according the output format
          ret = avcodec_encode_video2(pCodecCtxOut, &packet, outFrame, &got_output);
          if (got_output) {

            // dts, pts dance
            packet.dts = av_rescale_q_rnd(packet.dts,
                pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
                pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

            packet.pts = av_rescale_q_rnd(packet.pts,
                pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
                pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

            packet.duration = av_rescale_q(packet.duration,
                pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
                pFormatCtxOut->streams[video_stream_idx_cam]->time_base
                );





            // and write it to file
            //if(av_interleaved_write_frame(pFormatCtxOut,&packet) < 0){
            //  fprintf(stderr,"error writing frame");
            //  exit(1);
            //}
            // instead of writing to file, we write to socket


            int sent_size;
			if ((sent_size = send(SENDsocket, &packet, sizeof(&packet), 0)) == SOCKET_ERROR) {
				// error, terminate thread and exit
				printf("Send failed, terminate stream.\n");
				_endthreadex(0);
				return 1;
			}
			// printf("packet sent\n");



          }
          if (ret < 0){
            av_frame_free(&filt_frame);
            break;
          }
        }
      }
    }
    // free the packet everytime
    av_free_packet(&packet);
  }

  // flush out delayed frames
  for (got_output = 1; got_output; i++) {
    ret = avcodec_encode_video2(pCodecCtxOut, &packet, NULL, &got_output);
    if (ret < 0) {
      fprintf(stderr, "Error encoding frame\n");
      exit(1);
    }

    if (got_output) {
      packet.dts = av_rescale_q_rnd(packet.dts,
          pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
          pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      packet.pts = av_rescale_q_rnd(packet.pts,
          pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
          pFormatCtxOut->streams[video_stream_idx_cam]->time_base,
          AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

      packet.duration = av_rescale_q(packet.duration,
          pFormatCtxOut->streams[video_stream_idx_cam]->codec->time_base,
          pFormatCtxOut->streams[video_stream_idx_cam]->time_base
          );

      if(pCodecCtxOut->coded_frame->key_frame)
        packet.flags |= AV_PKT_FLAG_KEY;

      ret = av_interleaved_write_frame(pFormatCtxOut,&packet);
      if(ret < 0 ){
        exit(1);
      }
      av_free_packet(&packet);
    }
  }

  ret = av_write_trailer(pFormatCtxOut);
  if(ret < 0 ){
    exit(1);
  }












  /////////////// close everything
  avcodec_close(strmVdoOut->codec);
  avcodec_close(pCodecCtxInCam);
  avcodec_close(pCodecCtxOut);
  for(i = 0; i < pFormatCtxOut->nb_streams; i++) {
    av_freep(&pFormatCtxOut->streams[i]->codec);
    av_freep(&pFormatCtxOut->streams[i]);
  }
  avio_close(pFormatCtxOut->pb);
  av_free(pFormatCtxOut);
  av_free(pFormatCtxInCam);
  av_dict_free(&inOptions);





	/*
	// message to send to the client
	char *message = "Hey from the server!";
	int sent_size; // size of data sent
	// loop indefinitely to keep sending to the client until repeat set to fasle
	while (repeat) {
		// send data message to client
		if ((sent_size = send(SENDsocket, message, strlen(message), 0)) == SOCKET_ERROR) {
			// error, terminate thread and exit
			printf("Send failed, terminate stream.\n");
			_endthreadex(0);
			return 1;
		}
		// 5 seconds sleep to see what's happening in the terminal
		Sleep(5000L);
	}
	*/
	// connection interrupted by setting repeat to false, exit protocol
  printf("Connection interrupted.\n");

	// terminate thread as we are done with the stream
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
			ThreadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &SendStream, &SENDsocket, 0, NULL);

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
