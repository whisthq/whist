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
#include "../fractal/video/cpucapturetransfer.h"
#include "../fractal/video/screencapture.h"
#include "../fractal/video/videoencode.h"
#include "client.h"
#include "handle_client_message.h"
#include "network.h"

#ifdef _WIN32
#include "../fractal/utils/windows_utils.h"
#include "../fractal/video/dxgicudacapturetransfer.h"
#endif

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

#define USE_GPU 0
#define USE_MONITOR 0
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080

extern Client clients[MAX_NUM_CLIENTS];

volatile int connection_id;
static volatile bool connected;
static volatile bool running;
volatile double max_mbps;
volatile int client_width = -1;
volatile int client_height = -1;
volatile CodecType client_codec_type = CODEC_TYPE_UNKNOWN;
volatile bool update_device = true;
volatile FractalCursorID last_cursor;
input_device_t* input_device = NULL;
// volatile

char buf[LARGEST_FRAME_SIZE + sizeof(PeerUpdateMessage) * MAX_NUM_CLIENTS];

#define VIDEO_BUFFER_SIZE 25
#define MAX_VIDEO_INDEX 500
FractalPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];
int video_buffer_packet_len[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

#define AUDIO_BUFFER_SIZE 100
#define MAX_NUM_AUDIO_INDICES 3
FractalPacket audio_buffer[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];
int audio_buffer_packet_len[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];

SDL_mutex* packet_mutex;

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
CodecType encoder_factory_codec_type;
int32_t MultithreadedEncoderFactory(void* opaque) {
    opaque;
    encoder_factory_result = create_video_encoder(
        encoder_factory_server_w, encoder_factory_server_h,
        encoder_factory_client_w, encoder_factory_client_h,
        encoder_factory_current_bitrate, encoder_factory_codec_type);
    encoder_finished = true;
    return 0;
}
int32_t MultithreadedDestroyEncoder(void* opaque) {
    video_encoder_t* encoder = (video_encoder_t*)opaque;
    destroy_video_encoder(encoder);
    return 0;
}

int32_t SendVideo(void* opaque) {
    opaque;
    SDL_Delay(500);

#if defined(_WIN32)
    // set Windows DPI
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

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

#ifdef _WIN32
    bool dxgi_cuda_available = false;
#endif

    while (connected) {
        if (client_width < 0 || client_height < 0) {
            SDL_Delay(5);
            continue;
        }

        // Update device with new parameters
        if (update_device) {
            update_device = false;

#ifdef _WIN32
            // need to reinitialize this, so close it
            dxgi_cuda_close_transfer_context();
#endif

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
                encoder_factory_codec_type = (CodecType)client_codec_type;
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

#ifdef _WIN32
            if (encoder->type == NVENC_ENCODE) {
                // initialize the transfer context
                if (!dxgi_cuda_start_transfer_context(device)) {
                    dxgi_cuda_available = true;
                }
            } else if (dxgi_cuda_available) {
                // end the transfer context
                dxgi_cuda_close_transfer_context();
            }
#endif
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

            // transfer the screen to a buffer
            int transfer_res = 2;  // haven't tried anything yet
#if defined(_WIN32)
            if (encoder->type == NVENC_ENCODE && dxgi_cuda_available &&
                device->texture_on_gpu) {
                // if dxgi_cuda is setup and we have a dxgi texture on the gpu
                transfer_res = dxgi_cuda_transfer_capture(device, encoder);
            }
#endif
            if (transfer_res) {
                // if previous attempt failed or we need to use cpu
                transfer_res = cpu_transfer_capture(device, encoder);
            }
            if (transfer_res) {
                // if there was a failure
                connected = false;
                break;
            }

            clock t;
            StartTimer(&t);

            int res = video_encoder_encode(encoder);
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
                    frame->codec_type = encoder->codec_type;

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
                    PeerUpdateMessage* peer_update_msgs =
                        (PeerUpdateMessage*)(((char*)frame->compressed_frame) +
                                             frame->size);

                    size_t num_msgs;
                    if (readLock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-acquire is active RW lock.");
                    } else if (SDL_LockMutex(state_lock) != 0) {
                        LOG_ERROR("Failed to lock state lock");
                        if (readUnlock(&is_active_rwlock) != 0) {
                            LOG_ERROR(
                                "Failed to read-release is active RW lock.");
                        }
                    } else {
                        if (fillPeerUpdateMessages(peer_update_msgs,
                                                   &num_msgs) != 0) {
                            LOG_ERROR("Failed to copy peer update messages.");
                        }
                        frame->num_peer_update_msgs = (int)num_msgs;

                        StartTimer(&t);

                        // Send video packet to client
                        if (broadcastUDPPacket(
                                PACKET_VIDEO, (uint8_t*)frame,
                                frame_size +
                                    sizeof(PeerUpdateMessage) * (int)num_msgs,
                                id, STARTING_BURST_BITRATE,
                                video_buffer[id % VIDEO_BUFFER_SIZE],
                                video_buffer_packet_len[id %
                                                        VIDEO_BUFFER_SIZE]) !=
                            0) {
                            LOG_WARNING("Could not broadcast video frame ID %d",
                                        id);
                        } else {
                            // Only increment ID if the send succeeded
                            id++;
                        }
                        if (SDL_UnlockMutex(state_lock) != 0) {
                            LOG_ERROR("Failed to unlock state lock");
                        }
                        if (readUnlock(&is_active_rwlock) != 0) {
                            LOG_ERROR(
                                "Failed to read-release is active RW lock.");
                        }
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
    opaque;
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
    if (readLock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to read-acquire is active RW lock.");
    } else {
        if (broadcastUDPPacket(PACKET_MESSAGE, (uint8_t*)&fmsg, sizeof(fmsg), 1,
                               STARTING_BURST_BITRATE, NULL, NULL) != 0) {
            LOG_ERROR("Failed to broadcast audio packet.");
        }
        if (readUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-release is active RW lock.");
        }
    }
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
                    if (readLock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-acquire is active RW lock.");
                    } else {
                        if (broadcastUDPPacket(
                                PACKET_AUDIO, audio_encoder->encoded_frame_data,
                                audio_encoder->encoded_frame_size, id,
                                STARTING_BURST_BITRATE,
                                audio_buffer[id % AUDIO_BUFFER_SIZE],
                                audio_buffer_packet_len[id %
                                                        AUDIO_BUFFER_SIZE]) <
                            0) {
                            LOG_WARNING("Could not send audio frame");
                        }
                        if (readUnlock(&is_active_rwlock) != 0) {
                            LOG_ERROR(
                                "Failed to read-release is active RW lock.");
                        }
                    }
                    // mprintf("sent audio frame %d\n", id);
                    id++;

                    // Free encoder packet

                    av_packet_unref(&audio_encoder->packet);
                }
#else
                if (readLock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-acquire is active RW lock.");
                } else {
                    if (broadcastUDPPacket(
                            PACKET_AUDIO, audio_device->buffer,
                            audio_device->buffer_size, id,
                            STARTING_BURST_BITRATE,
                            audio_buffer[id % AUDIO_BUFFER_SIZE],
                            audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE]) <
                        0) {
                        mprintf("Could not send audio frame\n");
                    }
                    if (readUnlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-release is active RW lock.");
                    }
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

int doDiscoveryHandshake(SocketContext* context, int* client_id) {
    FractalPacket* packet;
    clock timer;
    StartTimer(&timer);
    do {
        packet = ReadTCPPacket(context);
        SDL_Delay(5);
    } while (packet == NULL && GetTimer(timer) < 3.0);
    if (packet == NULL) {
        LOG_WARNING("Did not receive discovery request from client.");
        closesocket(context->s);
        return -1;
    }

    FractalClientMessage* fcmsg = (FractalClientMessage*)packet->data;
    int username = fcmsg->discoveryRequest.username;

    if (readLock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to read-acquire is active RW lock.");
        return -1;
    }
    bool found;
    int ret;
    if ((ret = tryFindClientIdByUsername(username, &found, client_id)) != 0) {
        LOG_ERROR(
            "Failed to try to find client ID by username. "
            " (Username: %s)",
            username);
    }
    if (ret == 0 && found) {
        if (readUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-release is active RW lock.");
            closesocket(context->s);
            return -1;
        }
        if (writeLock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active RW lock.");
            closesocket(context->s);
            return -1;
        }
        ret = quitClient(*client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to quit client. (ID: %d)", *client_id);
        }
        if (writeUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
            closesocket(context->s);
            if (ret == 0) ret = -1;
        }
        if (ret != 0) return ret;
    } else {
        ret = getAvailableClientID(client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to find available client ID.");
            closesocket(context->s);
        }
        if (readUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-release is active RW lock.");
            return -1;
        }
        if (ret != 0) return -1;
    }

    clients[*client_id].username = username;
    LOG_INFO("Found ID for client. (ID: %d)", *client_id);

    size_t fsmsg_size =
        sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage* fsmsg = malloc(fsmsg_size);
    if (fsmsg == NULL) {
        LOG_ERROR("Failed to malloc server message.");
        return -1;
    }
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage* reply_msg =
        (FractalDiscoveryReplyMessage*)fsmsg->discovery_reply;

    reply_msg->client_id = *client_id;
    reply_msg->UDP_port = clients[*client_id].UDP_port;
    reply_msg->TCP_port = clients[*client_id].TCP_port;

    reply_msg->connection_id = connection_id;
    char* server_username = "Fractal";
    memcpy(reply_msg->username, server_username, strlen(server_username) + 1);
#ifdef _WIN32
    reply_msg->filename[0] = '\0';
    strcat(reply_msg->filename, "C:\\ProgramData\\FractalCache");
#else  // Linux
    char* cwd = getcwd(NULL, 0);
    memcpy(reply_msg->filename, cwd, strlen(cwd) + 1);
    free(cwd);
#endif

    LOG_INFO("Sending discovery packet");
    if (SendTCPPacket(context, PACKET_MESSAGE, (uint8_t*)fsmsg,
                      (int)fsmsg_size) < 0) {
        LOG_ERROR("Failed to send send discovery reply message.");
        closesocket(context->s);
        free(fsmsg);
        return -1;
    }

    closesocket(context->s);
    free(fsmsg);
    return 0;
}

int MultithreadedWaitForClient(void* opaque) {
    opaque;
    SocketContext discovery_context;
    int client_id;

    while (running) {
        if (CreateTCPContext(&discovery_context, NULL, PORT_DISCOVERY, 1, 5000,
                             USING_STUN) < 0) {
            continue;
        }

        if (doDiscoveryHandshake(&discovery_context, &client_id) != 0) {
            LOG_WARNING("Discovery handshake failed.");
            continue;
        }

        LOG_INFO("Discovery handshake succeeded. (ID: %d)", client_id);

        // Client is not in use so we don't need to worry about anyone else
        // touching it
        if (connectClient(client_id) != 0) {
            LOG_WARNING(
                "Failed to establish connection with client. "
                "(ID: %d)",
                client_id);
            continue;
        }

        LOG_INFO("Client connected. (ID: %d)", client_id);

        // We probably need to lock these. Argh.
        if (host_id == -1) {
            host_id = client_id;
        }
        num_active_clients++;
        if (num_controlling_clients == 0) {
            clients[client_id].is_controlling = true;
            num_controlling_clients++;
        }

        StartTimer(&(clients[client_id].last_ping));

        if (writeLock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active RW lock.");
            if (quitClient(client_id) != 0) {
                LOG_ERROR("Failed to quit client. (ID: %d)", client_id);
            }
            if (host_id == client_id) {
                // if (randomlyAssignHost() != 0) {
                //     LOG_ERROR("Failed to randomly assigned host.");
                // }
            }
            continue;
        }
        clients[client_id].is_active = true;
        if (writeUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
            LOG_ERROR("VERY BAD. IRRECOVERABLE.");
            if (quitClient(client_id) != 0) {
                LOG_ERROR("Failed to quit client. (ID: %d)", client_id);
            }
            if (host_id == client_id) {
                // if (randomlyAssignHost() != 0) {
                //     LOG_ERROR("Failed to randomly assigned host.");
                // }
            }
            // MAYBE RETURN (?)
            continue;
        }
    }

    return 0;
}

int main() {
    // int d = 0;
    // int e = 0;
    // LOG_INFO("d starts at %d, e starts at %d", d, e);
    // dxgi_cuda_transfer_data(&d, &e);
    // LOG_INFO("d set to %d, e set to %d", d, e);
    // return 0;
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
    LOG_INFO("Fractal server revision %s", FRACTAL_GIT_REVISION);

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);

// initialize the windows socket library if this is a windows client
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n",
                WSAGetLastError());
        destroyLogger();
        return -1;
    }
#endif

#ifdef _WIN32
    if (!InitDesktop()) {
        LOG_WARNING("Could not winlogon!\n");
        destroyLogger();
        return 0;
    }
#endif

    if (initClients() != 0) {
        LOG_ERROR("Failed to initialize client objects.");
        destroyLogger();
        return 1;
    }

    update();

    while (true) {
        updateStatus(true);

        clock startup_time;
        StartTimer(&startup_time);

        running = true;
        max_mbps = STARTING_BITRATE;
        wants_iframe = false;
        update_encoder = false;

        SDL_Thread* wait_for_client = SDL_CreateThread(
            MultithreadedWaitForClient, "MultithreadedWaitForClient", NULL);
        wait_for_client;
        while (num_active_clients == 0) SDL_Delay(500);
        connected = true;
        SDL_Delay(500);

        SDL_Thread* send_video = SDL_CreateThread(SendVideo, "SendVideo", NULL);
        SDL_Thread* send_audio = SDL_CreateThread(SendAudio, "SendAudio", NULL);
        LOG_INFO("Sending video and audio...");

        input_device = CreateInputDevice();
        if (!input_device) {
            LOG_WARNING("Failed to create input device for playback.");
        }

        clock totaltime;
        StartTimer(&totaltime);

        clock last_exit_check;
        StartTimer(&last_exit_check);

        clock last_ping_check;
        StartTimer(&last_ping_check);

        LOG_INFO("Receiving packets...");

        initClipboard();

        clock ack_timer;
        StartTimer(&ack_timer);

        while (connected) {
            if (GetTimer(ack_timer) > 5) {
#if USING_STUN
                // Broadcast ack
                if (readLock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-acquire is active RW lock.");
                } else {
                    if (broadcastAck() != 0) {
                        LOG_ERROR("Failed to broadcast acks.");
                    }
                    if (readUnlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-release is active RW lock.");
                    }
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
                if (readLock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-acquire is active RW lock.");
                } else {
                    if (broadcastTCPPacket(
                            PACKET_MESSAGE, (uint8_t*)fmsg_response,
                            sizeof(FractalServerMessage) + cb->size) < 0) {
                        LOG_WARNING("Could not broadcast Clipboard Message");
                    } else {
                        LOG_INFO("Send clipboard message!");
                    }
                    if (readUnlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-release is active RW lock.");
                    }
                }
                free(fmsg_response);
            }

            if (GetTimer(last_ping_check) > 20.0) {
                for (;;) {
                    if (readLock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-acquire is active RW lock.");
                        break;
                    }
                    bool exists, should_reap = false;
                    if (existsTimedOutClient(3.0, &exists) != 0) {
                        LOG_ERROR("Failed to find if a client has timed out.");
                    } else {
                        should_reap = exists;
                    }
                    if (readUnlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-release is active RW lock.");
                        break;
                    }
                    if (should_reap) {
                        if (writeLock(&is_active_rwlock) != 0) {
                            LOG_ERROR(
                                "Failed to write-acquire is active RW lock.");
                            break;
                        }
                        if (reapTimedOutClients(3.0) != 0) {
                            LOG_ERROR("Failed to reap timed out clients.");
                        }
                        if (writeUnlock(&is_active_rwlock) != 0) {
                            LOG_ERROR(
                                "Failed to write-release is active RW lock.");
                        }
                    }
                    break;
                }
                StartTimer(&last_ping_check);
            }

            if (GetTimer(last_exit_check) > 15.0 / 1000.0) {
// Exit file seen, time to exit
#ifdef _WIN32
                if (PathFileExistsA("C:\\Program Files\\Fractal\\Exit\\exit")) {
                    LOG_INFO("Exiting due to button press...");
                    FractalServerMessage fmsg_response = {0};
                    fmsg_response.type = SMESSAGE_QUIT;
                    if (readLock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-acquire is active RW lock.");
                    } else {
                        if (broadcastUDPPacket(
                                PACKET_MESSAGE, (uint8_t*)&fmsg_response,
                                sizeof(FractalServerMessage), 1,
                                STARTING_BURST_BITRATE, NULL, NULL) != 0) {
                            LOG_WARNING("Could not send Quit Message");
                        }
                        if (readUnlock(&is_active_rwlock) != 0) {
                            LOG_ERROR(
                                "Failed to read-release is active RW lock.");
                        }
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

            if (readLock(&is_active_rwlock) != 0) {
                LOG_ERROR("Failed to write-acquire is active lock.");
                continue;
            }
            for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
                if (!clients[id].is_active) continue;

                // Get packet!
                FractalClientMessage* fmsg;
                FractalClientMessage local_fcmsg;
                size_t fcmsg_size;
                if (tryGetNextMessageTCP(id, &fmsg, &fcmsg_size) != 0 ||
                    fcmsg_size == 0) {
                    if (tryGetNextMessageUDP(id, &local_fcmsg, &fcmsg_size) !=
                            0 ||
                        fcmsg_size == 0) {
                        continue;
                    }
                    fmsg = &local_fcmsg;
                }

                // HANDLE FRACTAL CLIENT MESSAGE
                if (SDL_LockMutex(state_lock) != 0) {
                    LOG_ERROR("Failed to lock state lock.");
                    continue;
                }
                bool is_controlling = clients[id].is_controlling;
                SDL_UnlockMutex(state_lock);
                if (handleClientMessage(fmsg, id, is_controlling) != 0) {
                    LOG_ERROR(
                        "Failed to handle message from client. "
                        "(ID: %d)",
                        id);
                } else {
                    // if (handleSpectatorMessage(fmsg, id) != 0) {
                    //     LOG_ERROR("Failed to handle message from spectator");
                    // }
                }
            }
            if (readUnlock(&is_active_rwlock) != 0) {
                LOG_ERROR("Failed to write-release is active lock.");
                continue;
            }
        }

        sendLogHistory();

        LOG_INFO("Disconnected");

        DestroyInputDevice(input_device);

        SDL_WaitThread(send_video, NULL);
        SDL_WaitThread(send_audio, NULL);
        SDL_DestroyMutex(packet_mutex);

        if (writeLock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active RW lock.");
            continue;
        }
        if (SDL_LockMutex(state_lock) != 0) {
            LOG_ERROR("Failed to lock state lock");
            if (writeUnlock(&is_active_rwlock) != 0) {
                LOG_ERROR("Failed to write-release is active RW lock.");
            }
            continue;
        }
        if (quitClients() != 0) {
            LOG_ERROR("Failed to quit clients.");
        }
        if (SDL_UnlockMutex(state_lock) != 0) {
            LOG_ERROR("Failed to unlock state lock");
        }
        if (writeUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
        }
    }

#ifdef _WIN32
    WSACleanup();
#endif

    destroyLogger();
    destroyClients();

    return 0;
}
