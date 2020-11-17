/**
 * Copyright Fractal Computers, Inc. 2020
 * @file main.c
 * @brief This file contains the main code that runs a Fractal server on a
Windows or Linux Ubuntu computer.
============================
Usage
============================

Follow main() to see a Fractal video streaming server being created and creating
its threads.
*/

/*
============================
Includes
============================
*/

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

#include "../fractal/utils/logging.h"
#include "../fractal/core/fractalgetopt.h"
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
#include "webserver.h"

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

#define BITS_IN_BYTE 8.0
#define TCP_CONNECTION_WAIT 5000
#define CLIENT_PING_TIMEOUT_SEC 3.0

extern Client clients[MAX_NUM_CLIENTS];

char binary_aes_private_key[16];
char hex_aes_private_key[33];
char identifier[FRACTAL_ENVIRONMENT_MAXLEN + 1];
char webserver_url[MAX_WEBSERVER_URL_LEN + 1];
volatile int connection_id;
static volatile bool exiting;

SDL_mutex* container_destruction_mutex;

volatile double max_mbps;
volatile int client_width = -1;
volatile int client_height = -1;
volatile int client_dpi = -1;
volatile CodecType client_codec_type = CODEC_TYPE_UNKNOWN;
volatile bool update_device = true;
volatile FractalCursorID last_cursor;
input_device_t* input_device = NULL;
extern char sentry_environment[FRACTAL_ENVIRONMENT_MAXLEN];
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

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Sends a message to the webserver to destroy
 *                                 the container running the server protocol.
 *
 * @returns                        Returns -1 on failure, 0 on success.
 */
int send_container_destroy_message();

/*
============================
Private Function Implementations
============================
*/

#ifdef __linux__
int xioerror_handler(Display* d) {
    /*
        When X display is destroyed, intercept XIOError in order to
        quit clients and send container destruction signal. Clients
        must be quit

        For use as arg for XSetIOErrorHandler - this is fatal and thus
        any program exit handling that would normally be expected to
        be handled in another thread must be explicitly handled here.
        Right now, we handle:
            * SendContainerDestroyMessage
            * quitClients
    */

    SDL_LockMutex(container_destruction_mutex);

    exiting = true;

    // Try sending a container destroy message - if this is the first destruction
    //  message being sent, then also quit all clients. Quitting clients will
    //  still happen on SendContainerDestroyMessage failure in case
    //  it is failure on the first attempt of sending a message. This means that there
    //  is a possibility of the quitClients() pipeline happening more than once
    //  if SendContainerDestroyMessage fails and then is called again because
    //  this error handler can be called multiple times.
    if (send_container_destroy_message() != 1) {
        // POSSIBLY below locks are not necessary if we're quitting everything and dying anyway?

        // Broadcast client quit message
        FractalServerMessage fmsg_response = {0};
        fmsg_response.type = SMESSAGE_QUIT;
        if (read_lock(&is_active_rwlock) != 0) {
            LOG_ERROR(
                "Failed to read-acquire is active RW "
                "lock.");
        } else {
            if (broadcast_udp_packet(PACKET_MESSAGE, (uint8_t*)&fmsg_response,
                                   sizeof(FractalServerMessage), 1, STARTING_BURST_BITRATE, NULL,
                                   NULL) != 0) {
                LOG_WARNING("Could not send Quit Message");
            }
            if (read_unlock(&is_active_rwlock) != 0) {
                LOG_ERROR("Failed to read-release is active RW lock.");
            }
        }

        // Kick all clients
        if (write_lock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active RW lock.");
            return -1;
        }
        if (SDL_LockMutex(state_lock) != 0) {
            LOG_ERROR("Failed to lock state lock");
            if (write_unlock(&is_active_rwlock) != 0) {
                LOG_ERROR("Failed to write-release is active RW lock.");
            }
            SDL_UnlockMutex(container_destruction_mutex);
            return -1;
        }
        if (quit_clients() != 0) {
            LOG_ERROR("Failed to quit clients.");
        }
        if (SDL_UnlockMutex(state_lock) != 0) {
            LOG_ERROR("Failed to unlock state lock");
        }
        if (write_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
        }
    }

    SDL_UnlockMutex(container_destruction_mutex);

    return 0;
}
#endif

int send_container_destroy_message() {
    /*
        Sends a message to the webserver to destroy the container on which
        the server is running. This will only work for contianers spun up using
        the webserver endpoint /container/create

        Surround this call by a mutex lock to be sure that it isn't sending a message twice.

        Returns:
            int: 0 on success, -1 on failure
    */

    // static, so only sets to false on first call
    static bool already_sent_destroy_message = false;

    if (already_sent_destroy_message) {
        return 0;
    }

    LOG_INFO("CONTAINER DESTROY SIGNAL");

    char* container_id = get_container_id();
    if (!container_id) {
        // If container_id is not found, then get protocol_info again
        LOG_INFO("Container ID not found, re-requesting protocol_info");
        update_webserver_parameters();
        container_id = get_container_id();

        // If container_id is still not found, then fail
        if (!container_id) {
            LOG_ERROR("Container destroy could not find container_id");
            return -1;
        }
    }

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\n"
             "  \"container_id\": \"%s\",\n"
             "  \"private_key\": \"%s\"\n"
             "}",
             container_id, hex_aes_private_key);

    // send destroy request, don't require response -> update this later
    char* resp_buf = NULL;
    size_t resp_buf_maxlen = 4800;
    send_post_request(webserver_url, "/container/delete", payload, &resp_buf, resp_buf_maxlen);

    LOG_INFO("/container/delete response: %s", resp_buf);

    already_sent_destroy_message = true;

    return 0;
}

int32_t multithreaded_encoder_factory(void* opaque) {
    opaque;
    encoder_factory_result = create_video_encoder(
        encoder_factory_server_w, encoder_factory_server_h, encoder_factory_client_w,
        encoder_factory_client_h, encoder_factory_current_bitrate, encoder_factory_codec_type);
    encoder_finished = true;
    return 0;
}
int32_t multithreaded_destroy_encoder(void* opaque) {
    video_encoder_t* encoder = (video_encoder_t*)opaque;
    destroy_video_encoder(encoder);
    return 0;
}

int32_t send_video(void* opaque) {
    opaque;
    SDL_Delay(500);

#if defined(_WIN32)
    // set Windows DPI
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    // Init DXGI Device
    CaptureDevice rdevice;
    CaptureDevice* device = NULL;

    init_cursors();

    // Init FFMPEG Encoder
    int current_bitrate = STARTING_BITRATE;
    video_encoder_t* encoder = NULL;

    double worst_fps = 40.0;
    int ideal_bitrate = current_bitrate;
    int bitrate_tested_frames = 0;
    int bytes_tested_frames = 0;

    clock previous_frame_time;
    start_timer(&previous_frame_time);
    int previous_frame_size = 0;

    // int consecutive_capture_screen_errors = 0;

    //    int defaultCounts = 1;

    clock world_timer;
    start_timer(&world_timer);

    int id = 1;
    update_device = true;

    clock last_frame_capture;
    start_timer(&last_frame_capture);

    pending_encoder = false;
    encoder_finished = false;

#ifdef _WIN32
    bool dxgi_cuda_available = false;
#endif

    while (!exiting) {
        if (num_active_clients == 0 || client_width < 0 || client_height < 0 || client_dpi < 0) {
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
                destroy_capture_device(device);
                device = NULL;
            }

            device = &rdevice;
            if (create_capture_device(device, client_width, client_height, client_dpi) < 0) {
                LOG_WARNING("Failed to create capture device");
                device = NULL;
                update_device = true;

                SDL_Delay(100);
                continue;
            }

            LOG_INFO("Created Capture Device of dimensions %dx%d", device->width, device->height);

            while (pending_encoder) {
                if (encoder_finished) {
                    if (encoder) {
                        SDL_CreateThread(multithreaded_destroy_encoder, "MultithreadedDestroyEncoder",
                                         encoder);
                    }
                    encoder = encoder_factory_result;
                    pending_encoder = false;
                    update_encoder = false;
                    break;
                }
                SDL_Delay(1);
            }
            update_encoder = true;
            if (encoder) {
                multithreaded_destroy_encoder(encoder);
                encoder = NULL;
            }
        }

        // Update encoder with new parameters
        if (update_encoder) {
            update_capture_encoder(device, current_bitrate, client_codec_type);
            // encoder = NULL;
            if (pending_encoder) {
                if (encoder_finished) {
                    if (encoder) {
                        SDL_CreateThread(multithreaded_destroy_encoder, "multithreaded_destroy_encoder",
                                         encoder);
                    }
                    encoder = encoder_factory_result;
                    pending_encoder = false;
                    update_encoder = false;
                }
            } else {
                LOG_INFO("Updating Encoder using Bitrate: %d from %f", current_bitrate, max_mbps);
                current_bitrate = (int)(max_mbps * 1024 * 1024);
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
                    multithreaded_encoder_factory(NULL);
                    encoder = encoder_factory_result;
                    pending_encoder = false;
                    update_encoder = false;
                } else {
                    SDL_CreateThread(multithreaded_encoder_factory, "multithreaded_encoder_factory",
                                     NULL);
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
        if (get_timer(last_frame_capture) > 1.0 / FPS) {
            accumulated_frames = capture_screen(device);
            // mprintf( "CaptureScreen: %d\n", accumulated_frames );
        }

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            LOG_WARNING("Failed to capture screen");

            destroy_capture_device(device);
            device = NULL;
            update_device = true;

            SDL_Delay(100);
            continue;
        }

        clock server_frame_timer;
        start_timer(&server_frame_timer);

        // Only if we have a frame to render
        if (accumulated_frames > 0 || wants_iframe ||
            get_timer(last_frame_capture) > 1.0 / MIN_FPS) {
            // LOG_INFO( "Frame Time: %f\n", get_timer( last_frame_capture ) );

            start_timer(&last_frame_capture);

            if (accumulated_frames > 1) {
                LOG_INFO("Accumulated Frames: %d", accumulated_frames);
            }
            if (accumulated_frames == 0) {
                LOG_INFO("Sending current frame!");
            }

            // transfer the screen to a buffer
            int transfer_res = 2;  // haven't tried anything yet
#if defined(_WIN32)
            if (encoder->type == NVENC_ENCODE && dxgi_cuda_available && device->texture_on_gpu) {
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
                exiting = true;
                break;
            }

            if (wants_iframe) {
                // True I-Frame is WIP
                LOG_ERROR("NOT GUARANTEED TO BE TRUE IFRAME");
                video_encoder_set_iframe(encoder);
                wants_iframe = false;
            }

            clock t;
            start_timer(&t);

            int res = video_encoder_encode(encoder);
            if (res < 0) {
                // bad boy error
                LOG_ERROR("Error encoding video frame!");
                exiting = true;
                break;
            } else if (res > 0) {
                // filter graph is empty
                break;
            }
            // else we have an encoded frame, so handle it!

            static int frame_stat_number = 0;
            static double total_frame_time = 0.0;
            static double max_frame_time = 0.0;
            static double total_frame_sizes = 0.0;
            static double max_frame_size = 0.0;

            frame_stat_number++;
            total_frame_time += get_timer(t);
            max_frame_time = max(max_frame_time, get_timer(t));
            total_frame_sizes += encoder->encoded_frame_size;
            max_frame_size = max(max_frame_size, encoder->encoded_frame_size);

            if (frame_stat_number % 30 == 0) {
                LOG_INFO("Longest Encode Time: %f", max_frame_time);
                LOG_INFO("Average Encode Time: %f", total_frame_time / 30);
                LOG_INFO("Longest Encode Size: %f", max_frame_size);
                LOG_INFO("Average Encode Size: %f", total_frame_sizes / 30);
                total_frame_time = 0.0;
                max_frame_time = 0.0;
                total_frame_sizes = 0.0;
                max_frame_size = 0.0;
            }

            video_encoder_unset_iframe(encoder);

            // mprintf("Encode Time: %f (%d) (%d)\n", get_timer(t),
            //        frames_since_first_iframe % gop_size,
            //        encoder->encoded_frame_size);

            bitrate_tested_frames++;
            bytes_tested_frames += encoder->encoded_frame_size;

            if (encoder->encoded_frame_size != 0) {
                double delay = -1.0;

                if (previous_frame_size > 0) {
                    double frame_time = get_timer(previous_frame_time);
                    start_timer(&previous_frame_time);
                    // double mbps = previous_frame_size * 8.0 / 1024.0 /
                    // 1024.0 / frame_time; TODO: bitrate throttling alg
                    // previousFrameSize * 8.0 / 1024.0 / 1024.0 / IdealTime
                    // = max_mbps previousFrameSize * 8.0 / 1024.0 / 1024.0
                    // / max_mbps = IdealTime
                    double transmit_time = ((double)previous_frame_size) * BITS_IN_BYTE /
                                           BYTES_IN_KILOBYTE / BYTES_IN_KILOBYTE / max_mbps;

                    // double average_frame_size = 1.0 * bytes_tested_frames
                    // / bitrate_tested_frames;
                    double current_trasmit_time = ((double)previous_frame_size) * BITS_IN_BYTE /
                                                  BYTES_IN_KILOBYTE / BYTES_IN_KILOBYTE / max_mbps;
                    double current_fps = 1.0 / current_trasmit_time;

                    delay = transmit_time - frame_time;
                    delay = min(delay, 0.004);

                    // mprintf("Size: %d, MBPS: %f, VS MAX MBPS: %f, Time:
                    // %f, Transmit Time: %f, Delay: %f\n",
                    // previous_frame_size, mbps, max_mbps, frame_time,
                    // transmit_time, delay);

                    if ((current_fps < worst_fps || ideal_bitrate > current_bitrate) &&
                        bitrate_tested_frames > 20) {
                        // Rather than having lower than the worst
                        // acceptable fps, find the ratio for what the
                        // bitrate should be
                        double ratio_bitrate = current_fps / worst_fps;
                        int new_bitrate = (int)(ratio_bitrate * current_bitrate);
                        if (abs(new_bitrate - current_bitrate) / new_bitrate > 0.05) {
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
                    frame->width = encoder->out_width;
                    frame->height = encoder->out_height;
                    frame->codec_type = encoder->codec_type;

                    frame->size = encoder->encoded_frame_size;
                    frame->cursor = get_current_cursor();
                    // True if this frame does not require previous frames to
                    // render
                    frame->is_iframe = encoder->is_iframe;
                    video_encoder_write_buffer(encoder, (void*)frame->compressed_frame);

                    // mprintf("Sent video packet %d (Size: %d) %s\n", id,
                    // encoder->encoded_frame_size, frame->is_iframe ?
                    // "(I-frame)" :
                    // "");
                    PeerUpdateMessage* peer_update_msgs =
                        (PeerUpdateMessage*)(((char*)frame->compressed_frame) + frame->size);

                    size_t num_msgs;
                    if (read_lock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-acquire is active RW lock.");
                    } else if (SDL_LockMutex(state_lock) != 0) {
                        LOG_ERROR("Failed to lock state lock");
                        if (read_unlock(&is_active_rwlock) != 0) {
                            LOG_ERROR("Failed to read-release is active RW lock.");
                        }
                    } else {
                        if (fill_peer_update_messages(peer_update_msgs, &num_msgs) != 0) {
                            LOG_ERROR("Failed to copy peer update messages.");
                        }
                        frame->num_peer_update_msgs = (int)num_msgs;

                        start_timer(&t);

                        // Send video packet to client
                        if (broadcast_udp_packet(
                                PACKET_VIDEO, (uint8_t*)frame,
                                frame_size + sizeof(PeerUpdateMessage) * (int)num_msgs, id,
                                STARTING_BURST_BITRATE, video_buffer[id % VIDEO_BUFFER_SIZE],
                                video_buffer_packet_len[id % VIDEO_BUFFER_SIZE]) != 0) {
                            LOG_WARNING("Could not broadcast video frame ID %d", id);
                        } else {
                            // Only increment ID if the send succeeded
                            id++;
                        }
                        if (SDL_UnlockMutex(state_lock) != 0) {
                            LOG_ERROR("Failed to unlock state lock");
                        }
                        if (read_unlock(&is_active_rwlock) != 0) {
                            LOG_ERROR("Failed to read-release is active RW lock.");
                        }
                    }

                    // LOG_INFO( "Send Frame Time: %f, Send Frame Size: %d\n",
                    // get_timer( t ), frame_size );

                    previous_frame_size = encoder->encoded_frame_size;
                    // double server_frame_time =
                    // get_timer(server_frame_timer); mprintf("Server Frame
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
    destroy_capture_device(device);
    device = NULL;
    multithreaded_destroy_encoder(encoder);
    encoder = NULL;

    return 0;
}

static int sample_rate = -1;

int32_t send_audio(void* opaque) {
    opaque;
    int id = 1;

    audio_device_t* audio_device = create_audio_device();
    if (!audio_device) {
        LOG_ERROR("Failed to create audio device...");
        return -1;
    }
    LOG_INFO("Created audio device!");
    start_audio_device(audio_device);
    audio_encoder_t* audio_encoder = create_audio_encoder(AUDIO_BITRATE, audio_device->sample_rate);
    int res;

    // Tell the client what audio frequency we're using
    sample_rate = audio_device->sample_rate;
    LOG_INFO("Audio Frequency: %d", audio_device->sample_rate);

    // setup

    while (!exiting) {
        // for each available packet
        for (get_next_packet(audio_device); packet_available(audio_device);
             get_next_packet(audio_device)) {
            get_buffer(audio_device);

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
                    if (read_lock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-acquire is active RW lock.");
                    } else {
                        if (broadcast_udp_packet(
                                PACKET_AUDIO, audio_encoder->encoded_frame_data,
                                audio_encoder->encoded_frame_size, id, STARTING_BURST_BITRATE,
                                audio_buffer[id % AUDIO_BUFFER_SIZE],
                                audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE]) < 0) {
                            LOG_WARNING("Could not send audio frame");
                        }
                        if (read_unlock(&is_active_rwlock) != 0) {
                            LOG_ERROR("Failed to read-release is active RW lock.");
                        }
                    }
                    // mprintf("sent audio frame %d\n", id);
                    id++;

                    // Free encoder packet

                    av_packet_unref(&audio_encoder->packet);
                }
#else
                if (read_lock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-acquire is active RW lock.");
                } else {
                    if (broadcast_udp_packet(PACKET_AUDIO, audio_device->buffer,
                                           audio_device->buffer_size, id, STARTING_BURST_BITRATE,
                                           audio_buffer[id % AUDIO_BUFFER_SIZE],
                                           audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE]) < 0) {
                        mprintf("Could not send audio frame\n");
                    }
                    if (readUnlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-release is active RW lock.");
                    }
                }
                id++;
#endif
            }

            release_buffer(audio_device);
        }
        wait_timer(audio_device);
    }

    // destroy_audio_encoder(audio_encoder);
    destroy_audio_device(audio_device);
    return 0;
}

#include <time.h>

int do_discovery_handshake(SocketContext* context, int* client_id) {
    FractalPacket* packet;
    clock timer;
    start_timer(&timer);
    do {
        packet = read_tcp_packet(context, true);
        SDL_Delay(5);
    } while (packet == NULL && get_timer(timer) < CLIENT_PING_TIMEOUT_SEC);
    if (packet == NULL) {
        LOG_WARNING("Did not receive discovery request from client.");
        closesocket(context->s);
        return -1;
    }

    FractalClientMessage* fcmsg = (FractalClientMessage*)packet->data;
    int username = fcmsg->discoveryRequest.username;

    if (read_lock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to read-acquire is active RW lock.");
        return -1;
    }
    bool found;
    int ret;
    if ((ret = try_find_client_id_by_username(username, &found, client_id)) != 0) {
        LOG_ERROR(
            "Failed to try to find client ID by username. "
            " (Username: %s)",
            username);
    }
    if (ret == 0 && found) {
        if (read_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-release is active RW lock.");
            closesocket(context->s);
            return -1;
        }
        if (write_lock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active RW lock.");
            closesocket(context->s);
            return -1;
        }
        ret = quit_client(*client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to quit client. (ID: %d)", *client_id);
        }
        if (write_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
            closesocket(context->s);
            if (ret == 0) ret = -1;
        }
        if (ret != 0) return ret;
    } else {
        ret = get_available_client_id(client_id);
        if (ret != 0) {
            LOG_ERROR("Failed to find available client ID.");
            closesocket(context->s);
        }
        if (read_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-release is active RW lock.");
            return -1;
        }
        if (ret != 0) return -1;
    }

    clients[*client_id].username = username;
    LOG_INFO("Found ID for client. (ID: %d)", *client_id);

    // TODO: Should check for is_controlling, but happens after this function call
    handle_client_message(fcmsg, *client_id, true);

    size_t fsmsg_size = sizeof(FractalServerMessage) + sizeof(FractalDiscoveryReplyMessage);

    FractalServerMessage* fsmsg = malloc(fsmsg_size);
    if (fsmsg == NULL) {
        LOG_ERROR("Failed to malloc server message.");
        return -1;
    }
    fsmsg->type = MESSAGE_DISCOVERY_REPLY;

    FractalDiscoveryReplyMessage* reply_msg = (FractalDiscoveryReplyMessage*)fsmsg->discovery_reply;

    reply_msg->client_id = *client_id;
    reply_msg->UDP_port = clients[*client_id].UDP_port;
    reply_msg->TCP_port = clients[*client_id].TCP_port;

    // Save connection ID
    save_connection_id(connection_id);

    // Send connection ID to client
    reply_msg->connection_id = connection_id;
    reply_msg->audio_sample_rate = sample_rate;
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
    LOG_INFO("Fsmsg size is %d", (int)fsmsg_size);
    if (send_tcp_packet(context, PACKET_MESSAGE, (uint8_t*)fsmsg, (int)fsmsg_size) < 0) {
        LOG_ERROR("Failed to send send discovery reply message.");
        closesocket(context->s);
        free(fsmsg);
        return -1;
    }

    closesocket(context->s);
    free(fsmsg);
    return 0;
}

int multithreaded_manage_clients(void* opaque) {
    opaque;

    SocketContext discovery_context;
    int client_id;

    bool trying_to_update = false;
    clock last_update_timer;
    start_timer(&last_update_timer);

    send_connection_history(webserver_url, identifier, hex_aes_private_key);
    connection_id = rand();
    start_connection_log();
    bool have_sent_logs = true;

    double nongraceful_grace_period = 600.0;  // 10 min after nongraceful disconn to reconn
    bool first_client_connected = false;      // set to true once the first client has connected
    double begin_time_to_exit = 60.0;  // client 1 min to connect when the server first goes up
    clock first_client_timer;  // start this now and then discard when first client has connected
    start_timer(&first_client_timer);

    while (!exiting) {
        if (read_lock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-acquire an active RW lock.");
            continue;
        }

        int saved_num_active_clients = num_active_clients;

        if (read_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read-release and active RW lock.");
            continue;
        }

        LOG_INFO("Num Active Clients %d, Have Sent Logs %s", saved_num_active_clients,
                 have_sent_logs ? "yes" : "no");
        if (saved_num_active_clients == 0 && !have_sent_logs) {
            send_connection_history(webserver_url, identifier, hex_aes_private_key);
            have_sent_logs = true;
        } else if (saved_num_active_clients > 0 && have_sent_logs) {
            have_sent_logs = false;
        }

        if (saved_num_active_clients == 0) {
            connection_id = rand();
            start_connection_log();

            if (trying_to_update) {
                if (get_timer(last_update_timer) > 10.0) {
                    update_webserver_parameters();
                    start_timer(&last_update_timer);
                }
            } else {
                start_timer(&last_update_timer);
                trying_to_update = true;
            }

            // container exit logic -
            //  * clients have connected before but now none are connected
            //  * no clients have connected in `begin_time_to_exit` secs of server being up
            // We don't place this in a lock because:
            //  * if the first client connects right on the threshold of begin_time_to_exit, it
            //  doesn't matter if we disconnect
            //  * if a new client connects right on the threshold of nongraceful_grace_period, it
            //  doesn't matter if we disconnect
            //  * if no clients are connected, it isn't possible for another client to nongracefully
            //  exit and reset the grace period timer
            if ((first_client_connected || (get_timer(first_client_timer) > begin_time_to_exit)) &&
                (!client_exited_nongracefully ||
                 (get_timer(last_nongraceful_exit) > nongraceful_grace_period))) {
                exiting = true;
            }
        } else {
            trying_to_update = false;

            // client has connected to server for the first time
            // update webserver parameters the first time a client connects
            if (!first_client_connected) {
                update_webserver_parameters();
                first_client_connected = true;
            }

            // nongraceful client grace period has ended, but clients are
            //  connected still - we don't want server to exit yet
            if (client_exited_nongracefully &&
                get_timer(last_nongraceful_exit) > nongraceful_grace_period) {
                client_exited_nongracefully = false;
            }
        }

        if (create_tcp_context(&discovery_context, NULL, PORT_DISCOVERY, 1, TCP_CONNECTION_WAIT,
                             get_using_stun(), binary_aes_private_key) < 0) {
            continue;
        }

        if (do_discovery_handshake(&discovery_context, &client_id) != 0) {
            LOG_WARNING("Discovery handshake failed.");
            continue;
        }

        LOG_INFO("Discovery handshake succeeded. (ID: %d)", client_id);

        // Client is not in use so we don't need to worry about anyone else
        // touching it
        if (connect_client(client_id, binary_aes_private_key) != 0) {
            LOG_WARNING(
                "Failed to establish connection with client. "
                "(ID: %d)",
                client_id);
            continue;
        }

        if (write_lock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active RW lock.");
            if (quit_client(client_id) != 0) {
                LOG_ERROR("Failed to quit client. (ID: %d)", client_id);
            }
            if (host_id == client_id) {
                // if (randomlyAssignHost() != 0) {
                //     LOG_ERROR("Failed to randomly assigned host.");
                // }
            }
            continue;
        }

        LOG_INFO("Client connected. (ID: %d)", client_id);

        // We probably need to lock these. Argh.
        if (host_id == -1) {
            host_id = client_id;
        }

        num_active_clients++;
        /* Make everyone a controller */
        clients[client_id].is_controlling = true;
        num_controlling_clients++;
        // if (num_controlling_clients == 0) {
        //     clients[client_id].is_controlling = true;
        //     num_controlling_clients++;
        // }

        if (clients[client_id].is_controlling) {
            // Reset input system when a new input controller arrives
            reset_input();
        }

        // reapTimedOutClients is called within a writeLock(&is_active_rwlock) and therefore this
        // should as well
        //  reapTimedOutClients only ever writes client_exited_nongracefully as true. This thread
        //  only writes it as false.
        if (client_exited_nongracefully &&
            (get_timer(last_nongraceful_exit) > nongraceful_grace_period)) {
            client_exited_nongracefully = false;
        }

        start_timer(&(clients[client_id].last_ping));

        clients[client_id].is_active = true;

        if (write_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
            LOG_ERROR("VERY BAD. IRRECOVERABLE.");
            if (quit_client(client_id) != 0) {
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

// required_argument means --identifier MUST take an argument
const struct option cmd_options[] = {{"private-key", required_argument, NULL, 'k'},
                                     {"identifier", required_argument, NULL, 'i'},
                                     {"webserver", required_argument, NULL, 'w'},
                                     // these are standard for POSIX programs
                                     {"help", no_argument, NULL, FRACTAL_GETOPT_HELP_CHAR},
                                     {"version", no_argument, NULL, FRACTAL_GETOPT_VERSION_CHAR},
                                     // end with NULL-termination
                                     {0, 0, 0, 0}};

// i: means --identifier MUST take an argument
#define OPTION_STRING "k:i:w:"

int parse_args(int argc, char* argv[]) {
    // TODO: replace `server` with argv[0]
    const char* usage =
        "Usage: server [OPTION]... IP_ADDRESS\n"
        "Try 'server --help' for more information.\n";
    const char* usage_details =
        "Usage: server [OPTION]... IP_ADDRESS\n"
        "\n"
        "All arguments to both long and short options are mandatory.\n"
        // regular options should look nice, with 2-space indenting for multiple lines
        "  -k, --private-key=PK          Pass in the RSA Private Key as a\n"
        "                                  hexadecimal string. Defaults to\n"
        "                                  binary and hex default keys in\n"
        "                                  the protocol code\n"
        "  -i, --identifier=ID           Pass in the unique identifier for this\n"
        "                                  server as a hexadecimal string\n"
        "  -w, --webserver=WS_URL        Pass in the webserver url for this\n"
        "                                  server's requests\n"
        // special options should be indented further to the left
        "      --help     Display this help and exit\n"
        "      --version  Output version information and exit\n";
    memcpy((char*)&binary_aes_private_key, DEFAULT_BINARY_PRIVATE_KEY,
           sizeof(binary_aes_private_key));
    memcpy((char*)&hex_aes_private_key, DEFAULT_HEX_PRIVATE_KEY, sizeof(hex_aes_private_key));

    int opt;

    while (true) {
        opt = getopt_long(argc, argv, OPTION_STRING, cmd_options, NULL);
        if (opt != -1 && optarg && strlen(optarg) > FRACTAL_ENVIRONMENT_MAXLEN) {
            printf("Option passed into %c is too long! Length of %zd when max is %d", opt,
                   strlen(optarg), FRACTAL_ENVIRONMENT_MAXLEN);
            return -1;
        }
        errno = 0;
        switch (opt) {
            case 'k': {
                if (!read_hexadecimal_private_key(optarg, (char*)binary_aes_private_key,
                                                  (char*)hex_aes_private_key)) {
                    printf("Invalid hexadecimal string: %s\n", optarg);
                    printf("%s", usage);
                    return -1;
                }
                break;
            }
            case 'i': {
                printf("Identifier passed in: %s\n", optarg);
                if (strlen(optarg) > FRACTAL_IDENTIFIER_MAXLEN) {
                    printf("Identifier passed in is too long! Has length %lu but max is %d.\n",
                           (unsigned long)strlen(optarg), FRACTAL_IDENTIFIER_MAXLEN);
                    return -1;
                }
                strncpy(identifier, optarg, FRACTAL_IDENTIFIER_MAXLEN);
                identifier[FRACTAL_IDENTIFIER_MAXLEN] = 0;
                break;
            }
            case 'w': {
                printf("Webserver URL passed in: %s\n", optarg);
                if (strlen(optarg) > MAX_WEBSERVER_URL_LEN) {
                    printf("Webserver url passed in is too long! Has length %lu but max is %d.\n",
                           (unsigned long)strlen(optarg), MAX_WEBSERVER_URL_LEN);
                }
                strncpy(webserver_url, optarg, MAX_WEBSERVER_URL_LEN);
                webserver_url[MAX_WEBSERVER_URL_LEN] = 0;
                break;
            }
            case FRACTAL_GETOPT_HELP_CHAR: {
                printf("%s", usage_details);
                return 1;
            }
            case FRACTAL_GETOPT_VERSION_CHAR: {
                printf("Fractal client revision %s\n", FRACTAL_GIT_REVISION);
                return 1;
            }
            default: {
                if (opt != -1) {
                    // illegal option
                    printf("%s", usage);
                    return -1;
                }
                break;
            }
        }
        if (opt == -1) {
            bool can_accept_nonoption_args = false;
            if (optind < argc && can_accept_nonoption_args) {
                // there's a valid non-option arg
                // Do stuff with argv[optind]
                ++optind;
            } else if (optind < argc && !can_accept_nonoption_args) {
                // incorrect usage
                printf("%s", usage);
                return -1;
            } else {
                // we're done
                break;
            }
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int ret = parse_args(argc, argv);
    if (ret == -1) {
        // invalid usage
        return -1;
    } else if (ret == 1) {
        // --help or --version
        return 0;
    }

    init_default_port_mappings();

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

    srand((unsigned int)time(NULL));
    connection_id = rand();
    // set env to dev and update later
    strcpy(sentry_environment, "dev");
#ifdef _WIN32
    init_logger("C:\\ProgramData\\FractalCache");
#else
    init_logger(".");
#endif
    LOG_INFO("Version Number: %s", get_version());
    LOG_INFO("Fractal server revision %s", FRACTAL_GIT_REVISION);

    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);

// initialize the windows socket library if this is a windows client
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        destroy_logger();
        return -1;
    }
#endif

    input_device = create_input_device();
    if (!input_device) {
        LOG_WARNING("Failed to create input device for playback.");
    }

#ifdef _WIN32
    if (!InitDesktop(input_device, get_vm_password())) {
        LOG_WARNING("Could not winlogon!\n");
        destroy_logger();
        return 0;
    }
#endif

    if (init_clients() != 0) {
        LOG_ERROR("Failed to initialize client objects.");
        destroy_logger();
        return 1;
    }

    container_destruction_mutex = SDL_CreateMutex();
#ifdef __linux__
    XSetIOErrorHandler(xioerror_handler);
#endif

    update_server_status(false, webserver_url, identifier, hex_aes_private_key);

    // webserver will not know about this container until updateServerStatus is called above
    update_webserver_parameters();

    clock startup_time;
    start_timer(&startup_time);

    max_mbps = STARTING_BITRATE;
    wants_iframe = false;
    update_encoder = false;
    exiting = false;

    SDL_Thread* manage_clients_thread =
        SDL_CreateThread(multithreaded_manage_clients, "MultithreadedManageClients", NULL);
    SDL_Delay(500);

    SDL_Thread* send_video_thread = SDL_CreateThread(send_video, "send_video", NULL);
    SDL_Thread* send_audio_thread = SDL_CreateThread(send_audio, "send_audio", NULL);
    LOG_INFO("Sending video and audio...");

    clock totaltime;
    start_timer(&totaltime);

    clock last_exit_check;
    start_timer(&last_exit_check);

    clock last_ping_check;
    start_timer(&last_ping_check);

    LOG_INFO("Receiving packets...");

    init_clipboard_synchronizer();

    clock ack_timer;
    start_timer(&ack_timer);

    while (!exiting) {
        if (get_timer(ack_timer) > 5) {
            if (get_using_stun()) {
                // Broadcast ack
                if (read_lock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-acquire is active RW lock.");
                } else {
                    if (broadcast_ack() != 0) {
                        LOG_ERROR("Failed to broadcast acks.");
                    }
                    if (read_unlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to read-release is active RW lock.");
                    }
                }
            }
            update_server_status(num_controlling_clients > 0, webserver_url, identifier,
                               hex_aes_private_key);
            start_timer(&ack_timer);
        }

        // If they clipboard as updated, we should send it over to the
        // client
        ClipboardData* cb = clipboard_synchronizer_get_new_clipboard();
        if (cb) {
            LOG_INFO("Received clipboard trigger! Sending to client");
            FractalServerMessage* fmsg_response = malloc(sizeof(FractalServerMessage) + cb->size);
            fmsg_response->type = SMESSAGE_CLIPBOARD;
            memcpy(&fmsg_response->clipboard, cb, sizeof(ClipboardData) + cb->size);
            if (read_lock(&is_active_rwlock) != 0) {
                LOG_ERROR("Failed to read-acquire is active RW lock.");
            } else {
                if (broadcast_tcp_packet(PACKET_MESSAGE, (uint8_t*)fmsg_response,
                                       sizeof(FractalServerMessage) + cb->size) < 0) {
                    LOG_WARNING("Could not broadcast Clipboard Message");
                } else {
                    LOG_INFO("Send clipboard message!");
                }
                if (read_unlock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-release is active RW lock.");
                }
            }
            free(fmsg_response);
        }

        if (get_timer(last_ping_check) > 20.0) {
            for (;;) {
                if (read_lock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-acquire is active RW lock.");
                    break;
                }
                bool exists, should_reap = false;
                if (exists_timed_out_client(CLIENT_PING_TIMEOUT_SEC, &exists) != 0) {
                    LOG_ERROR("Failed to find if a client has timed out.");
                } else {
                    should_reap = exists;
                }
                if (read_unlock(&is_active_rwlock) != 0) {
                    LOG_ERROR("Failed to read-release is active RW lock.");
                    break;
                }
                if (should_reap) {
                    if (write_lock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to write-acquire is active RW lock.");
                        break;
                    }
                    if (reap_timed_out_clients(CLIENT_PING_TIMEOUT_SEC) != 0) {
                        LOG_ERROR("Failed to reap timed out clients.");
                    }
                    if (write_unlock(&is_active_rwlock) != 0) {
                        LOG_ERROR("Failed to write-release is active RW lock.");
                    }
                }
                break;
            }
            start_timer(&last_ping_check);
        }

        if (read_lock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-acquire is active lock.");
            continue;
        }

        for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
            if (!clients[id].is_active) continue;

            // Get packet!
            FractalClientMessage* fmsg;
            FractalClientMessage local_fcmsg;
            size_t fcmsg_size;
            if (try_get_next_message_tcp(id, &fmsg, &fcmsg_size) != 0 || fcmsg_size == 0) {
                if (try_get_next_message_udp(id, &local_fcmsg, &fcmsg_size) != 0 || fcmsg_size == 0) {
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
            if (handle_client_message(fmsg, id, is_controlling) != 0) {
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
        if (read_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active lock.");
            continue;
        }
    }

    destroy_input_device(input_device);
    destroy_clipboard_synchronizer();

    SDL_WaitThread(send_video_thread, NULL);
    SDL_WaitThread(send_audio_thread, NULL);
    SDL_WaitThread(manage_clients_thread, NULL);

    SDL_DestroyMutex(packet_mutex);

    if (write_lock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to write-acquire is active RW lock.");
        return -1;
    }
    if (SDL_LockMutex(state_lock) != 0) {
        LOG_ERROR("Failed to lock state lock");
        if (write_unlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
        }
        return -1;
    }
    if (quit_clients() != 0) {
        LOG_ERROR("Failed to quit clients.");
    }
    if (SDL_UnlockMutex(state_lock) != 0) {
        LOG_ERROR("Failed to unlock state lock");
    }
    if (write_unlock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to write-release is active RW lock.");
    }

#ifdef _WIN32
    WSACleanup();
#endif

    destroy_logger();
    destroy_clients();

    SDL_LockMutex(container_destruction_mutex);
    if (send_container_destroy_message() == -1) {
        SDL_UnlockMutex(container_destruction_mutex);
        return -1;
    }
    SDL_UnlockMutex(container_destruction_mutex);

    return 0;
}
