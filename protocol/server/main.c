/*
 * Fractal Server.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#include <shlwapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#endif

#include "../fractal/audio/audiocapture.h"
#include "../fractal/audio/audioencode.h"
#include "../fractal/core/fractal.h"
#include "../fractal/cursor/cursor.h"
#include "../fractal/input/input.h"
#include "../fractal/network/network.h"
#include "../fractal/utils/aes.h"
#include "../fractal/utils/logging.h"
#include "../fractal/video/screencapture.h"
#include "../fractal/video/videoencode.h"

#ifdef _WIN32
#include "../fractal/utils/windows_utils.h"
#endif

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

#define USE_GPU 0
#define USE_MONITOR 0
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080

volatile int connection_id;
static volatile bool connected;
static volatile double max_mbps;
volatile int client_width = -1;
volatile int client_height = -1;
volatile bool update_device = true;
volatile FractalCursorID last_cursor;
// volatile

char buf[LARGEST_FRAME_SIZE];

#define VIDEO_BUFFER_SIZE 25
#define MAX_VIDEO_INDEX 500
FractalPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];
int video_buffer_packet_len[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

#define AUDIO_BUFFER_SIZE 100
#define MAX_NUM_AUDIO_INDICES 3
FractalPacket audio_buffer[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];
int audio_buffer_packet_len[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];

SDL_mutex* packet_mutex;

#define MAX_SPECTATOR_CONNECTIONS 100

int num_spectator_connections;
SocketContext PacketSendContext = {0};
SocketContext SpectatorSendContext[MAX_SPECTATOR_CONNECTIONS];

volatile bool wants_iframe;
volatile bool update_encoder;

bool pending_encoder;
bool encoder_finished;
video_encoder_t* encoder_factory_result = NULL;

int encoder_factory_server_w;
int encoder_factory_server_h;
int encoder_factory_client_w;
int encoder_factory_client_h;
int encoder_factory_current_bitrate;
int32_t MultithreadedEncoderFactory(void* opaque) {
    opaque;
    encoder_factory_result =
        create_video_encoder(encoder_factory_server_w, encoder_factory_server_h,
                             encoder_factory_client_w, encoder_factory_client_h,
                             encoder_factory_current_bitrate);
    encoder_finished = true;
    return 0;
}
int32_t MultithreadedDestroyEncoder(void* opaque) {
    video_encoder_t* encoder = (video_encoder_t*)opaque;
    destroy_video_encoder(encoder);
    return 0;
}

int32_t SendVideo(void* opaque) {
    SDL_Delay(500);

#if defined(_WIN32)
    // set Windows DPI
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    SocketContext socketContext = *(SocketContext*)opaque;

    // Init DXGI Device
    CaptureDevice rdevice;
    CaptureDevice* device = NULL;

    InitCursors();

    // Init FFMPEG Encoder
    int current_bitrate = STARTING_BITRATE;
    video_encoder_t* encoder = NULL;

    double worst_fps = 40.0;
    int ideal_bitrate = current_bitrate;
    int bitrate_tested_frames = 0;
    int bytes_tested_frames = 0;

    clock previous_frame_time;
    StartTimer(&previous_frame_time);
    int previous_frame_size = 0;

    // int consecutive_capture_screen_errors = 0;

    //    int defaultCounts = 1;

    clock world_timer;
    StartTimer(&world_timer);

    int id = 1;
    int frames_since_first_iframe = 0;
    update_device = true;

    clock last_frame_capture;
    StartTimer(&last_frame_capture);

    pending_encoder = false;
    encoder_finished = false;

    while (connected) {
        if (client_width < 0 || client_height < 0) {
            SDL_Delay(5);
            continue;
        }

        // Update device with new parameters
        if (update_device) {
            update_device = false;

            if (device) {
                DestroyCaptureDevice(device);
                device = NULL;
            }

            device = &rdevice;
            if (CreateCaptureDevice(device, client_width, client_height) < 0) {
                LOG_WARNING("Failed to create capture device");
                device = NULL;
                update_device = true;

                SDL_Delay(100);
                continue;
            }

            LOG_INFO("Created Capture Device of dimensions %dx%d",
                     device->width, device->height);

            update_encoder = true;
            if (encoder) {
                MultithreadedDestroyEncoder(encoder);
                encoder = NULL;
            }
        }

        // Update encoder with new parameters
        if (update_encoder) {
            // encoder = NULL;
            if (pending_encoder) {
                if (encoder_finished) {
                    if (encoder) {
                        SDL_CreateThread(MultithreadedDestroyEncoder,
                                         "MultithreadedDestroyEncoder",
                                         encoder);
                    }
                    encoder = encoder_factory_result;
                    frames_since_first_iframe = 0;
                    pending_encoder = false;
                    update_encoder = false;
                }
            } else {
                current_bitrate = (int)(STARTING_BITRATE);
                LOG_INFO("Updating Encoder using Bitrate: %d from %f\n",
                         current_bitrate, max_mbps);
                pending_encoder = true;
                encoder_finished = false;
                encoder_factory_server_w = device->width;
                encoder_factory_server_h = device->height;
                encoder_factory_client_w = (int)client_width;
                encoder_factory_client_h = (int)client_height;
                encoder_factory_current_bitrate = current_bitrate;
                if (encoder == NULL) {
                    // Run on this thread bc we have to wait for it anyway
                    MultithreadedEncoderFactory(NULL);
                    encoder = encoder_factory_result;
                    frames_since_first_iframe = 0;
                    pending_encoder = false;
                    update_encoder = false;
                } else {
                    SDL_CreateThread(MultithreadedEncoderFactory,
                                     "MultithreadedEncoderFactory", NULL);
                }
            }
        }

        // Accumulated_frames is equal to how many frames have passed since the
        // last call to CaptureScreen
        int accumulated_frames = 0;
        if (GetTimer(last_frame_capture) > 1.0 / FPS) {
            accumulated_frames = CaptureScreen(device);
            // mprintf( "CaptureScreen: %d\n", accumulated_frames );
        }

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            LOG_WARNING("Failed to capture screen");

            DestroyCaptureDevice(device);
            device = NULL;
            update_device = true;

            SDL_Delay(100);
            continue;
        }

        clock server_frame_timer;
        StartTimer(&server_frame_timer);

        // Only if we have a frame to render
        if (accumulated_frames > 0 || wants_iframe ||
            GetTimer(last_frame_capture) > 1.0 / MIN_FPS) {
            // LOG_INFO( "Frame Time: %f\n", GetTimer( last_frame_capture ) );

            StartTimer(&last_frame_capture);

            if (accumulated_frames > 1) {
                LOG_INFO("Accumulated Frames: %d", accumulated_frames);
            }
            if (accumulated_frames == 0) {
                LOG_INFO("Sending current frame!");
            }

            bool is_iframe = false;
            if (frames_since_first_iframe % encoder->gop_size == 0) {
                wants_iframe = false;
                is_iframe = true;
            } else if (wants_iframe) {
                video_encoder_set_iframe(encoder);
                wants_iframe = false;
                is_iframe = true;
            }

            clock t;
            StartTimer(&t);

            int res = video_encoder_encode(encoder, device->frame_data,
                                           device->pitch);
            if (res < 0) {
                // bad boy error
                LOG_ERROR("Error encoding video frame!");
                connected = false;
                break;
            } else if (res > 0) {
                // filter graph is empty
                break;
            }
            // else we have an encoded frame, so handle it!

            frames_since_first_iframe++;

            static int frame_stat_number = 0;
            static double total_frame_time = 0.0;
            static double max_frame_time = 0.0;
            static double total_frame_sizes = 0.0;
            static double max_frame_size = 0.0;

            frame_stat_number++;
            total_frame_time += GetTimer(t);
            max_frame_time = max(max_frame_time, GetTimer(t));
            total_frame_sizes += encoder->encoded_frame_size;
            max_frame_size = max(max_frame_size, encoder->encoded_frame_size);

            if (frame_stat_number % 30 == 0) {
                LOG_INFO("Longest Encode Time: %f\n", max_frame_time);
                LOG_INFO("Average Encode Time: %f\n", total_frame_time / 30);
                LOG_INFO("Longest Encode Size: %f\n", max_frame_size);
                LOG_INFO("Average Encode Size: %f\n", total_frame_sizes / 30);
                total_frame_time = 0.0;
                max_frame_time = 0.0;
                total_frame_sizes = 0.0;
                max_frame_size = 0.0;
            }

            video_encoder_unset_iframe(encoder);

            // mprintf("Encode Time: %f (%d) (%d)\n", GetTimer(t),
            //        frames_since_first_iframe % gop_size,
            //        encoder->encoded_frame_size);

            bitrate_tested_frames++;
            bytes_tested_frames += encoder->encoded_frame_size;

            if (encoder->encoded_frame_size != 0) {
                double delay = -1.0;

                if (previous_frame_size > 0) {
                    double frame_time = GetTimer(previous_frame_time);
                    StartTimer(&previous_frame_time);
                    // double mbps = previous_frame_size * 8.0 / 1024.0 /
                    // 1024.0 / frame_time; TODO: bitrate throttling alg
                    // previousFrameSize * 8.0 / 1024.0 / 1024.0 / IdealTime
                    // = max_mbps previousFrameSize * 8.0 / 1024.0 / 1024.0
                    // / max_mbps = IdealTime
                    double transmit_time =
                        previous_frame_size * 8.0 / 1024.0 / 1024.0 / max_mbps;

                    // double average_frame_size = 1.0 * bytes_tested_frames
                    // / bitrate_tested_frames;
                    double current_trasmit_time =
                        previous_frame_size * 8.0 / 1024.0 / 1024.0 / max_mbps;
                    double current_fps = 1.0 / current_trasmit_time;

                    delay = transmit_time - frame_time;
                    delay = min(delay, 0.004);

                    // mprintf("Size: %d, MBPS: %f, VS MAX MBPS: %f, Time:
                    // %f, Transmit Time: %f, Delay: %f\n",
                    // previous_frame_size, mbps, max_mbps, frame_time,
                    // transmit_time, delay);

                    if ((current_fps < worst_fps ||
                         ideal_bitrate > current_bitrate) &&
                        bitrate_tested_frames > 20) {
                        // Rather than having lower than the worst
                        // acceptable fps, find the ratio for what the
                        // bitrate should be
                        double ratio_bitrate = current_fps / worst_fps;
                        int new_bitrate =
                            (int)(ratio_bitrate * current_bitrate);
                        if (abs(new_bitrate - current_bitrate) / new_bitrate >
                            0.05) {
                            // LOG_INFO("Updating bitrate from %d to %d",
                            //        current_bitrate, new_bitrate);
                            // TODO: Analyze bitrate handling with GOP size
                            // current_bitrate = new_bitrate;
                            // update_encoder = true;

                            bitrate_tested_frames = 0;
                            bytes_tested_frames = 0;
                        }
                    }
                }

                int frame_size = sizeof(Frame) + encoder->encoded_frame_size;
                if (frame_size > LARGEST_FRAME_SIZE) {
                    LOG_WARNING("Frame too large: %d", frame_size);
                } else {
                    // Create frame struct with compressed frame data and
                    // metadata
                    Frame* frame = (Frame*)buf;
                    frame->width = encoder->pCodecCtx->width;
                    frame->height = encoder->pCodecCtx->height;

                    frame->size = encoder->encoded_frame_size;
                    frame->cursor = GetCurrentCursor();
                    // True if this frame does not require previous frames to
                    // render
                    frame->is_iframe = is_iframe;
                    video_encoder_write_buffer(encoder,
                                               (void*)frame->compressed_frame);

                    // mprintf("Sent video packet %d (Size: %d) %s\n", id,
                    // encoder->encoded_frame_size, frame->is_iframe ?
                    // "(I-frame)" :
                    // "");

                    StartTimer(&t);

                    // Send video packet to client
                    if (SendUDPPacket(
                            &socketContext, PACKET_VIDEO, (uint8_t*)frame,
                            frame_size, id, STARTING_BURST_BITRATE,
                            video_buffer[id % VIDEO_BUFFER_SIZE],
                            video_buffer_packet_len[id % VIDEO_BUFFER_SIZE]) <
                        0) {
                        LOG_WARNING("Could not send video frame ID %d", id);
                    } else {
                        for( int i = 0; i < num_spectator_connections; i++ )
                        {
                            if( SendUDPPacket(
                                &SpectatorSendContext[i],
                                PACKET_VIDEO, (uint8_t*)frame,
                                frame_size, id, STARTING_BURST_BITRATE,
                                NULL,
                                NULL ) <
                                0 )
                            {
                                LOG_WARNING( "Could not send video frame ID %d to spectator %d", id, i );
                            }
                        }
                        // Only increment ID if the send succeeded
                        id++;
                    }

                    // LOG_INFO( "Send Frame Time: %f, Send Frame Size: %d\n",
                    // GetTimer( t ), frame_size );

                    previous_frame_size = encoder->encoded_frame_size;
                    // double server_frame_time =
                    // GetTimer(server_frame_timer); mprintf("Server Frame
                    // Time for ID %d: %f\n", id, server_frame_time);
                }
            }
        }
    }
#ifdef _WIN32
    HCURSOR new_cursor = LoadCursor(NULL, IDC_ARROW);

    SetSystemCursor(new_cursor, last_cursor);
#else
// TODO: Linux cursor instead
#endif
    DestroyCaptureDevice(device);
    device = NULL;
    MultithreadedDestroyEncoder(encoder);
    encoder = NULL;

    return 0;
}

int32_t SendAudio(void* opaque) {
    SocketContext context = *(SocketContext*)opaque;
    int id = 1;

    audio_device_t* audio_device = CreateAudioDevice();
    if (!audio_device) {
        LOG_ERROR("Failed to create audio device...");
        return -1;
    }
    LOG_INFO("Created audio device!");
    StartAudioDevice(audio_device);
    audio_encoder_t* audio_encoder =
        create_audio_encoder(AUDIO_BITRATE, audio_device->sample_rate);
    int res;

    // Tell the client what audio frequency we're using

    FractalServerMessage fmsg;
    fmsg.type = MESSAGE_AUDIO_FREQUENCY;
    fmsg.frequency = audio_device->sample_rate;
    SendUDPPacket(&PacketSendContext, PACKET_MESSAGE, (uint8_t*)&fmsg,
                  sizeof(fmsg), 1, STARTING_BURST_BITRATE, NULL, NULL);
    LOG_INFO("Audio Frequency: %d", audio_device->sample_rate);

    // setup

    while (connected) {
        // for each available packet
        for (GetNextPacket(audio_device); PacketAvailable(audio_device);
             GetNextPacket(audio_device)) {
            GetBuffer(audio_device);

            if (audio_device->buffer_size > 10000) {
                LOG_WARNING("Audio buffer size too large!");
            } else if (audio_device->buffer_size > 0) {
#if USING_AUDIO_ENCODE_DECODE

                // add samples to encoder fifo

                audio_encoder_fifo_intake(audio_encoder, audio_device->buffer,
                                          audio_device->frames_available);

                // while fifo has enough samples for an aac frame, handle it
                while (av_audio_fifo_size(audio_encoder->pFifo) >=
                       audio_encoder->pCodecCtx->frame_size) {
                    // create and encode a frame

                    res = audio_encoder_encode_frame(audio_encoder);

                    if (res < 0) {
                        // bad boy error
                        LOG_WARNING("error encoding packet");
                        continue;
                    } else if (res > 0) {
                        // no data or need more data
                        break;
                    }

                    // mprintf("we got a packet of size %d\n",
                    //         audio_encoder->encoded_frame_size);

                    // Send packet

                    if (SendUDPPacket(
                            &context, PACKET_AUDIO,
                            audio_encoder->encoded_frame_data,
                            audio_encoder->encoded_frame_size, id,
                            STARTING_BURST_BITRATE,
                            audio_buffer[id % AUDIO_BUFFER_SIZE],
                            audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE]) <
                        0) {
                        LOG_WARNING("Could not send audio frame");
                    }
                    // mprintf("sent audio frame %d\n", id);
                    id++;

                    // Free encoder packet

                    av_packet_unref(&audio_encoder->packet);
                }
#else
                if (SendUDPPacket(
                        &context, PACKET_AUDIO, audio_device->buffer,
                        audio_device->buffer_size, id, STARTING_BURST_BITRATE,
                        audio_buffer[id % AUDIO_BUFFER_SIZE],
                        audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE]) < 0) {
                    mprintf("Could not send audio frame\n");
                }
                id++;
#endif
            }

            ReleaseBuffer(audio_device);
        }
        WaitTimer(audio_device);
    }

    // destroy_audio_encoder(audio_encoder);
    DestroyAudioDevice(audio_device);
    return 0;
}

void SetTimezoneFromUtc(int utc, int DST_flag){
    if (DST_flag > 0){
        utc = utc - 1;
    }
    char* timezone;
    char cmd[5000];
//    Open powershell ' here closing ' in timezone
    snprintf(cmd, 17, "powershell.exe 'Set-TimeZone -Id ");
    switch(utc){
        case -12:
            timezone = " 'Dateline Standard Time' ' \0";
            break;
        case -11:
            timezone = " 'UTC-11' ' \0";
            break;
        case -10:
            timezone = " 'Hawaiian Standard Time' ' \0";
            break;
        case -9:
            timezone = " 'Alaskan Standard Time' ' \0";
            break;
        case -8:
            timezone = " 'Pacific Standard Time' ' \0";
            break;
        case -7:
            timezone = " 'Mountain Standard Time' ' \0";
            break;
        case -6:
            timezone = " 'Central Standard Time' '\0";
            break;
        case -5:
            timezone = " 'US Eastern Standard Time' '\0";
            break;
        case -4:
            timezone = " 'Atlantic Standard Time' ' \0";
            break;
        case -3:
            timezone = " ' E. South America Standard Time' '\0";
            break;
        case -2:
            timezone = " 'Mid-Atlantic Standard Time'  '\0";
            break;
        case -1:
            timezone = " 'Cape Verde Standard Time'  '\0";
            break;
        case 0:
            timezone = " 'GMT Standard Time'  '\0";
            break;
        case 1:
            timezone = " 'W. Europe Standard Time' '\0";
            break;
        case 2:
            timezone = " 'E. Europe Standard Time' ' \0";
            break;
        case 3:
            timezone = " 'Turkey Standard Time' ' \0";
            break;
        case 4:
            timezone = " 'Arabian Standard Time' ' \0";
            break;
        default:
            LOG_WARNING("Note a valid UTC offset: %d", utc);
            return;
    }
    snprintf(cmd + strlen(cmd), strlen(timezone), timezone);
    char* response[1000];
    runcmd(cmd, response);
    printf("command %s \n", cmd);
    printf("response %s\n", response);
}


void update() {
    if (is_dev_vm()) {
        LOG_INFO("dev vm - not auto-updating");
    } else {
        if (!get_branch()) {
            LOG_ERROR("COULD NOT GET BRANCH");
            return;
        }

        LOG_INFO("Checking for server protocol updates...");
        char cmd[5000];

        snprintf(cmd, sizeof(cmd),
#ifdef _WIN32
                 "powershell -command \"iwr -outf 'C:\\Program "
                 "Files\\Fractal\\update.bat' "
                 "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/%s/"
                 "update.bat\"",
                 get_branch()
#else
                 "TODO: Linux command?"
#endif
        );

        runcmd(cmd, NULL);

        snprintf(cmd, sizeof(cmd),
                 "cmd.exe /C \"C:\\Program Files\\Fractal\\update.bat\" %s",
                 get_branch());

        runcmd(
#ifdef _WIN32
            cmd
#else
            "TODO: Linux command?"
#endif
            ,
            NULL);
    }
}

#include <time.h>

int MultithreadedWaitForSpectator( void* opaque ) {
    opaque;
    while (connected) {
        SocketContext socket;
        if (CreateUDPContext(&socket,
                             NULL, PORT_SPECTATOR,
                             1, 5000,
                             USING_STUN) < 0) {
            LOG_INFO("Waiting for spectator");
            continue;
        }

        FractalServerMessage fmsg;
        fmsg.type = MESSAGE_INIT;
        fmsg.spectator_port = PORT_SPECTATOR + 1 + num_spectator_connections;

        if (SendUDPPacket(&socket, PACKET_MESSAGE,
                          (uint8_t*)&fmsg,
                          sizeof(FractalServerMessage), 1, -1, NULL, NULL) < 0) {
            LOG_ERROR("Could not send spectator init message!");
            return -1;
        }

        closesocket( socket.s );

        LOG_INFO("SPECTATOR #%d HANDSHAKE!", num_spectator_connections);
        if (CreateUDPContext(&SpectatorSendContext[num_spectator_connections],
                             NULL, PORT_SPECTATOR + 1 + num_spectator_connections,
                             1, 5000, USING_STUN) < 0) {
            LOG_INFO("Waiting for spectator");
            continue;
        }
        LOG_INFO("SPECTATOR #%d CONNECTED!", num_spectator_connections);

        num_spectator_connections++;
    }
    return 0;
}

int main() {
    //    static_assert(sizeof(unsigned short) == 2,
    //                  "Error: Unsigned short is not length 2 bytes!\n");

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

    srand((unsigned int)time(NULL));
    connection_id = rand();
#ifdef _WIN32
    initLogger("C:\\ProgramData\\FractalCache");
#else
    initLogger(".");
#endif
    LOG_INFO("Version Number: %s", get_version());

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);

// initialize the windows socket library if this is a windows client
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n",
                WSAGetLastError());
        return -1;
    }
#endif

#ifdef _WIN32
    if (!InitDesktop()) {
        LOG_WARNING("Could not winlogon!\n");
        return 0;
    }
#endif

    update();

    while (true) {
        srand(rand() * (unsigned int)time(NULL) + rand());
        connection_id = rand();

        num_spectator_connections = 0;
        SocketContext PacketReceiveContext = {0};
        SocketContext PacketTCPContext = {0};

        updateStatus(false);

        if (CreateUDPContext(&PacketReceiveContext, NULL, PORT_CLIENT_TO_SERVER,
                             1, 5000, USING_STUN) < 0) {
            LOG_WARNING("Failed to start connection");

            // Since we're just idling, let's try updating the server
            update();

            continue;
        }

        if (CreateUDPContext(&PacketSendContext, NULL, PORT_SERVER_TO_CLIENT, 1,
                             500, USING_STUN) < 0) {
            LOG_WARNING(
                "Failed to finish connection (Failed at port server to "
                "client).");
            closesocket(PacketReceiveContext.s);
            continue;
        }

        if (CreateTCPContext(&PacketTCPContext, NULL, PORT_SHARED_TCP, 1, 500,
                             USING_STUN) < 0) {
            LOG_WARNING("Failed to finish connection (Failed at TCP context).");
            closesocket(PacketReceiveContext.s);
            closesocket(PacketSendContext.s);
            continue;
        }

        updateStatus(true);

        // Give client time to setup before sending it with packets
        SDL_Delay(150);

        FractalServerMessage* msg_init_whole = malloc(
            sizeof(FractalServerMessage) + sizeof(FractalServerMessageInit));
        msg_init_whole->type = MESSAGE_INIT;
        FractalServerMessageInit* msg_init =
            (FractalServerMessageInit*)msg_init_whole->init_msg;
#ifdef _WIN32
        msg_init->filename[0] = '\0';
        strcat(msg_init->filename, "C:\\ProgramData\\FractalCache");
        char* username = "Fractal";
#else  // Linux
        char* cwd = getcwd(NULL, 0);
        memcpy(msg_init->filename, cwd, strlen(cwd) + 1);
        free(cwd);
        char* username = "Fractal";
#endif
        msg_init->connection_id = connection_id;
        memcpy(msg_init->username, username, strlen(username) + 1);
        LOG_INFO("SIZE: %d", sizeof(FractalServerMessage) +
                                 sizeof(FractalServerMessageInit));
        packet_mutex = SDL_CreateMutex();

        if (SendTCPPacket(&PacketTCPContext, PACKET_MESSAGE,
                          (uint8_t*)msg_init_whole,
                          sizeof(FractalServerMessage) +
                              sizeof(FractalServerMessageInit)) < 0) {
            LOG_ERROR("Could not send server init message!");
            return -1;
        }
        free(msg_init_whole);

        clock startup_time;
        StartTimer(&startup_time);

        connected = true;
        max_mbps = STARTING_BITRATE;
        wants_iframe = false;
        update_encoder = false;

        SDL_Thread* send_video =
            SDL_CreateThread(SendVideo, "SendVideo", &PacketSendContext);
        SDL_Thread* send_audio =
            SDL_CreateThread(SendAudio, "SendAudio", &PacketSendContext);
        LOG_INFO("Sending video and audio...");
        SDL_Thread* collect_spectators =
            SDL_CreateThread(MultithreadedWaitForSpectator,
                             "MultithreadedWaitForSpectator", NULL);
        collect_spectators;

        input_device_t* input_device = CreateInputDevice();
        if (!input_device) {
            LOG_WARNING("Failed to create input device for playback.");
        }

        FractalClientMessage local_fmsg;
        FractalClientMessage* fmsg;

        clock last_ping;
        StartTimer(&last_ping);

        clock totaltime;
        StartTimer(&totaltime);

        clock last_exit_check;
        StartTimer(&last_exit_check);

        LOG_INFO("Receiving packets...");

        int last_input_id = -1;
        initClipboard();

        clock ack_timer;
        StartTimer(&ack_timer);

        while (connected) {
            if (GetTimer(ack_timer) > 5) {
#if USING_STUN
                Ack(&PacketTCPContext);
                Ack(&PacketSendContext);
                Ack(&PacketReceiveContext);
                for (int i = 0; i < num_spectator_connections; i++) {
                    Ack(&SpectatorSendContext[i]);
                }
#endif
                updateStatus(true);
                StartTimer(&ack_timer);
            }

            // If they clipboard as updated, we should send it over to the
            // client
            if (hasClipboardUpdated()) {
                FractalServerMessage* fmsg_response = malloc(10000000);
                fmsg_response->type = SMESSAGE_CLIPBOARD;
                ClipboardData* cb = GetClipboard();
                memcpy(&fmsg_response->clipboard, cb,
                       sizeof(ClipboardData) + cb->size);
                LOG_INFO("Received clipboard trigger! Sending to client");
                if (SendTCPPacket(&PacketTCPContext, PACKET_MESSAGE,
                                  (uint8_t*)fmsg_response,
                                  sizeof(FractalServerMessage) + cb->size) <
                    0) {
                    LOG_WARNING("Could not send Clipboard Message");
                } else {
                    LOG_INFO("Send clipboard message!");
                }
                free(fmsg_response);
            }

            if (GetTimer(last_ping) > 3.0) {
                LOG_WARNING("Client connection dropped.");
                connected = false;
            }

            if (GetTimer(last_exit_check) > 15.0 / 1000.0) {
// Exit file seen, time to exit
#ifdef _WIN32
                if (PathFileExistsA("C:\\Program Files\\Fractal\\Exit\\exit")) {
                    LOG_INFO("Exiting due to button press...");
                    FractalServerMessage fmsg_response = {0};
                    fmsg_response.type = SMESSAGE_QUIT;
                    if (SendUDPPacket(&PacketSendContext, PACKET_MESSAGE,
                                      (uint8_t*)&fmsg_response,
                                      sizeof(FractalServerMessage), 1,
                                      STARTING_BURST_BITRATE, NULL, NULL) < 0) {
                        LOG_WARNING("Could not send Quit Message");
                    }
                    // Give a bit of time to make sure no one is touching it
                    SDL_Delay(50);
                    DeleteFileA("C:\\Program Files\\Fractal\\Exit\\exit");
                    connected = false;
                }
#else
// TODO: Filesystem for Unix
#endif
                StartTimer(&last_exit_check);
            }

            // START Get Packet
            FractalPacket* tcp_packet = ReadTCPPacket(&PacketTCPContext);
            if (tcp_packet) {
                fmsg = (FractalClientMessage*)tcp_packet->data;
                LOG_INFO("Received TCP BUF!!!! Size %d",
                         tcp_packet->payload_size);
                LOG_INFO("Received %d byte clipboard message from client.",
                         tcp_packet->payload_size);
            } else {
                memset(&local_fmsg, 0, sizeof(local_fmsg));

                fmsg = &local_fmsg;

                FractalPacket* decrypted_packet =
                    ReadUDPPacket(&PacketReceiveContext);

                for (int i = 0;
                     i < num_spectator_connections && !decrypted_packet; i++) {
                    LOG_INFO("READING FROM %d", i);
                    SDL_Delay(50);
                    FractalPacket* spectator_decrypted_packet =
                        ReadUDPPacket(&SpectatorSendContext[i]);
                    if (spectator_decrypted_packet) {
                        FractalClientMessage* fcmsg =
                            (void*)spectator_decrypted_packet->data;
                        if (fcmsg->type == MESSAGE_IFRAME_REQUEST) {
                            LOG_INFO("Iframe requested from spectator!");
                            decrypted_packet = spectator_decrypted_packet;
                        }
                    }
                }

                if (decrypted_packet) {
                    // Copy data into an fmsg
                    memcpy(fmsg, decrypted_packet->data,
                           max(sizeof(*fmsg),
                               (size_t)decrypted_packet->payload_size));

                    // Check to see if decrypted packet is of valid size
                    if (decrypted_packet->payload_size != GetFmsgSize(fmsg)) {
                        LOG_WARNING("Packet is of the wrong size!: %d",
                                    decrypted_packet->payload_size);
                        LOG_WARNING("Type: %d", fmsg->type);
                        fmsg->type = 0;
                    }

                    // Make sure that keyboard events are played in order
                    if (fmsg->type == MESSAGE_KEYBOARD ||
                        fmsg->type == MESSAGE_KEYBOARD_STATE) {
                        // Check that id is in order
                        if (decrypted_packet->id > last_input_id) {
                            decrypted_packet->id = last_input_id;
                        } else {
                            // Received keyboard input out of order, just
                            // ignore
                            fmsg->type = 0;
                        }
                    }
                } else {
                    fmsg->type = 0;
                }
            }
            // END Get Packet

            if (fmsg->type != 0) {
                if (fmsg->type == MESSAGE_KEYBOARD ||
                    fmsg->type == MESSAGE_MOUSE_BUTTON ||
                    fmsg->type == MESSAGE_MOUSE_WHEEL ||
                    fmsg->type == MESSAGE_MOUSE_MOTION) {
                    // Replay user input (keyboard or mouse presses)
                    if (input_device) {
                        if (!ReplayUserInput(input_device, fmsg)) {
                            LOG_WARNING("Failed to replay input!");
#ifdef _WIN32
                            InitDesktop();
#endif
                        }
                    }

                } else if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
// TODO: Unix version missing
// Synchronize client and server keyboard state
#ifdef _WIN32
                    UpdateKeyboardState(input_device, fmsg);
#endif
                } else if (fmsg->type == MESSAGE_MBPS) {
                    // Update mbps
                    LOG_INFO("MSG RECEIVED FOR MBPS: %f\n", fmsg->mbps);
                    max_mbps =
                        max(fmsg->mbps, MINIMUM_BITRATE / 1024.0 / 1024.0);
                    // update_encoder = true;
                } else if (fmsg->type == MESSAGE_PING) {
                    LOG_INFO("Ping Received - ID %d", fmsg->ping_id);

                    // Response to ping with pong
                    FractalServerMessage fmsg_response = {0};
                    fmsg_response.type = MESSAGE_PONG;
                    fmsg_response.ping_id = fmsg->ping_id;
                    StartTimer(&last_ping);
                    if (SendUDPPacket(&PacketSendContext, PACKET_MESSAGE,
                                      (uint8_t*)&fmsg_response,
                                      sizeof(fmsg_response), 1,
                                      STARTING_BURST_BITRATE, NULL, NULL) < 0) {
                        LOG_WARNING("Could not send Ping");
                    }
                } else if (fmsg->type == MESSAGE_DIMENSIONS) {
                    LOG_INFO("Request to use dimensions %dx%d received",
                             fmsg->dimensions.width, fmsg->dimensions.height);
                    // Update knowledge of client monitor dimensions
                    if (client_width != fmsg->dimensions.width ||
                        client_height != fmsg->dimensions.height) {
                        client_width = fmsg->dimensions.width;
                        client_height = fmsg->dimensions.height;
                        // Update device if knowledge changed
                        update_device = true;
                    }
                } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
                    // Update clipboard with message
                    LOG_INFO("Received Clipboard Data! %d",
                             fmsg->clipboard.type);
                    SetClipboard(&fmsg->clipboard);
                } else if (fmsg->type == MESSAGE_AUDIO_NACK) {
                    // Audio nack received, relay the packet

                    // mprintf("Audio NACK requested for: ID %d Index %d\n",
                    // fmsg->nack_data.id, fmsg->nack_data.index);
                    FractalPacket* audio_packet =
                        &audio_buffer[fmsg->nack_data.id % AUDIO_BUFFER_SIZE]
                                     [fmsg->nack_data.index];
                    int len = audio_buffer_packet_len[fmsg->nack_data.id %
                                                      AUDIO_BUFFER_SIZE]
                                                     [fmsg->nack_data.index];
                    if (audio_packet->id == fmsg->nack_data.id) {
                        LOG_INFO(
                            "NACKed audio packet %d found of length %d. "
                            "Relaying!",
                            fmsg->nack_data.id, len);
                        ReplayPacket(&PacketSendContext, audio_packet, len);
                    }
                    // If we were asked for an invalid index, just ignore it
                    else if (fmsg->nack_data.index <
                             audio_packet->num_indices) {
                        LOG_WARNING(
                            "NACKed audio packet %d %d not found, ID %d %d was "
                            "located instead.",
                            fmsg->nack_data.id, fmsg->nack_data.index,
                            audio_packet->id, audio_packet->index);
                    }
                } else if (fmsg->type == MESSAGE_VIDEO_NACK) {
                    // Video nack received, relay the packet

                    // mprintf("Video NACK requested for: ID %d Index %d\n",
                    // fmsg->nack_data.id, fmsg->nack_data.index);
                    FractalPacket* video_packet =
                        &video_buffer[fmsg->nack_data.id % VIDEO_BUFFER_SIZE]
                                     [fmsg->nack_data.index];
                    int len = video_buffer_packet_len[fmsg->nack_data.id %
                                                      VIDEO_BUFFER_SIZE]
                                                     [fmsg->nack_data.index];
                    if (video_packet->id == fmsg->nack_data.id) {
                        LOG_INFO(
                            "NACKed video packet ID %d Index %d found of "
                            "length %d. Relaying!",
                            fmsg->nack_data.id, fmsg->nack_data.index, len);
                        ReplayPacket(&PacketSendContext, video_packet, len);
                    }

                    // If we were asked for an invalid index, just ignore it
                    else if (fmsg->nack_data.index <
                             video_packet->num_indices) {
                        LOG_WARNING(
                            "NACKed video packet %d %d not found, ID %d %d was "
                            "located instead.",
                            fmsg->nack_data.id, fmsg->nack_data.index,
                            video_packet->id, video_packet->index);
                    }
                } else if (fmsg->type == MESSAGE_IFRAME_REQUEST) {
                    LOG_INFO("Request for i-frame found: Creating iframe");
                    if (fmsg->reinitialize_encoder) {
                        update_encoder = true;
                    } else {
                        wants_iframe = true;
                    }
                } else if (fmsg->type == CMESSAGE_QUIT) {
                    // Client requested to exit, it's time to disconnect
                    LOG_INFO("Client Quit");
                    connected = false;
                } else if (fmsg->type == MESSAGE_TIME){
                    LOG_INFO("Recieving a message time packet, client has offset: %d", fmsg->time_data.UTC_Offset);
                    SetTimezoneFromUtc( fmsg->time_data.UTC_Offset, fmsg->time_data.DST_flag);
                    printf("Client UTC offset %d\n", fmsg->time_data.UTC_Offset);
                }
            }
        }

        sendLogHistory();

        LOG_INFO("Disconnected");

        DestroyInputDevice(input_device);

        SDL_WaitThread(send_video, NULL);
        SDL_WaitThread(send_audio, NULL);
        SDL_DestroyMutex(packet_mutex);

        closesocket(PacketReceiveContext.s);
        closesocket(PacketSendContext.s);
        closesocket(PacketTCPContext.s);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    destroyLogger();

    return 0;
}
