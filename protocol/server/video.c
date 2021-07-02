/**
 * Copyright Fractal Computers, Inc. 2021
 * @file video.c
 * @brief This file contains all code that interacts directly with processing
 *        video on the server.
============================
Usage
============================

multithreaded_send_video() is called on its own thread and loops repeatedly to capture and send
video.
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
#include <signal.h>
#include <unistd.h>
#endif

#include <fractal/video/transfercapture.h>
#include <fractal/video/screencapture.h>
#include <fractal/video/videoencode.h>
#include <fractal/utils/avpacket_buffer.h>
#include "client.h"
#include "network.h"
#include "video.h"

#ifdef _WIN32
#include <fractal/utils/windows_utils.h>
#endif

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

#define USE_GPU 0
#define USE_MONITOR 0

#define BITS_IN_BYTE 8.0

extern volatile bool exiting;

extern volatile double max_mbps;
volatile int client_width = -1;
volatile int client_height = -1;
volatile int client_dpi = -1;
volatile CodecType client_codec_type = CODEC_TYPE_UNKNOWN;
volatile bool update_device = true;

#define VIDEO_BUFFER_SIZE 25
#define MAX_VIDEO_INDEX 500
FractalPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];
int video_buffer_packet_len[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

extern volatile bool wants_iframe;
extern volatile bool update_encoder;

static bool pending_encoder;
static bool encoder_finished;
static VideoEncoder* encoder_factory_result = NULL;
// If we are using nvidia's built in encoder, then we do not need to create a real encoder
// struct since frames will already be encoded, So we can use this dummy encoder.
static VideoEncoder dummy_encoder = {0};

static int encoder_factory_server_w;
static int encoder_factory_server_h;
static int encoder_factory_client_w;
static int encoder_factory_client_h;
static int encoder_factory_current_bitrate;
static CodecType encoder_factory_codec_type;

/*
============================
Private Functions
============================
*/

int32_t multithreaded_encoder_factory(void* opaque);
int32_t multithreaded_destroy_encoder(void* opaque);

/*
============================
Private Function Implementations
============================
*/

int32_t multithreaded_encoder_factory(void* opaque) {
    UNUSED(opaque);
    encoder_factory_result = create_video_encoder(
        encoder_factory_server_w, encoder_factory_server_h, encoder_factory_client_w,
        encoder_factory_client_h, encoder_factory_current_bitrate, encoder_factory_codec_type);
    encoder_finished = true;
    return 0;
}

int32_t multithreaded_destroy_encoder(void* opaque) {
    VideoEncoder* encoder = (VideoEncoder*)opaque;
    if (encoder != &dummy_encoder) {
        destroy_video_encoder(encoder);
    }
    return 0;
}

/*
============================
Public Function Implementations
============================
*/

int32_t multithreaded_send_video(void* opaque) {
    UNUSED(opaque);
    fractal_set_thread_priority(FRACTAL_THREAD_PRIORITY_REALTIME);
    fractal_sleep(500);

#if defined(_WIN32)
    // set Windows DPI
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
#endif

    // Capture Device
    CaptureDevice rdevice;
    CaptureDevice* device = NULL;

    init_cursors();

    // Init FFMPEG Encoder
    int current_bitrate = STARTING_BITRATE;
    VideoEncoder* encoder = NULL;

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

    while (!exiting) {
        if (num_active_clients == 0 || client_width < 0 || client_height < 0 || client_dpi < 0) {
            fractal_sleep(5);
            continue;
        }

        // Update device with new parameters
        if (update_device) {
            update_device = false;

            if (device) {
                destroy_capture_device(device);
                device = NULL;
            }

            device = &rdevice;
            // YUV pixel format requires the width to be a multiple of 4 and the height to be a
            // multiple of 2 (see `bRoundFrameSize` in NvFBC.h). By default, the dimensions will be
            // implicitly rounded up, but for some reason it looks better if we explicitly set the
            // size. Also for some reason it actually rounds the width to a multiple of 8.
            int true_width = client_width + 7 - ((client_width + 7) % 8);
            int true_height = client_height + 1 - ((client_height + 1) % 2);
            if (create_capture_device(device, true_width, true_height, client_dpi, current_bitrate,
                                      client_codec_type) < 0) {
                LOG_WARNING("Failed to create capture device");
                device = NULL;
                update_device = true;

                fractal_sleep(100);
                continue;
            }

            LOG_INFO("Created Capture Device of dimensions %dx%d", device->width, device->height);

            if (device->using_nvidia) {
                // The frames will already arrive encoded by the GPU, so we do not need to do any
                // encoding ourselves
                encoder = &dummy_encoder;
                memset(encoder, 0, sizeof(VideoEncoder));
                update_encoder = false;
            } else {
                // If an encoder is pending, while capture_device is updating, then we should wait
                // for it to be created
                while (pending_encoder) {
                    if (encoder_finished) {
                        encoder = encoder_factory_result;
                        pending_encoder = false;
                        break;
                    }
                    fractal_sleep(1);
                }
                // If an encoder exists, then we should destroy it since the capture device is being
                // created now
                if (encoder) {
                    fractal_create_thread(multithreaded_destroy_encoder,
                                          "multithreaded_destroy_encoder", encoder);
                    encoder = NULL;
                }
                // Next, we should update our ffmpeg encoder
                update_encoder = true;
            }
        }

        // Update encoder with new parameters
        if (update_encoder) {
            if (device->using_nvidia) {
                // If this device uses a device encoder, then we should update it
                update_capture_encoder(device, current_bitrate, client_codec_type);
                // We keep the dummy encoder as-is,
                // the real encoder was just updated with update_capture_encoder
                update_encoder = false;
            } else {
                // Keep track of whether or not a new encoder is being used now
                bool new_encoder_used = false;

                // Otherwise, this capture device must use an external encoder,
                // so we should start making it in our encoder factory
                if (pending_encoder) {
                    if (encoder_finished) {
                        // Once encoder_finished, we'll destroy the old one that we've been using,
                        // and replace it with the result of multithreaded_encoder_factory
                        if (encoder) {
                            fractal_create_thread(multithreaded_destroy_encoder,
                                                  "multithreaded_destroy_encoder", encoder);
                        }
                        encoder = encoder_factory_result;
                        pending_encoder = false;
                        update_encoder = false;

                        new_encoder_used = true;
                    }
                } else {
                    // Starting making new encoder. This will set pending_encoder=true, but won't
                    // actually update it yet, we'll still use the old one for a bit
                    LOG_INFO("Updating Encoder using Bitrate: %d from %f", current_bitrate,
                             max_mbps);
                    current_bitrate = (int)(max_mbps * 1024 * 1024);
                    encoder_finished = false;
                    encoder_factory_server_w = device->width;
                    encoder_factory_server_h = device->height;
                    encoder_factory_client_w = (int)client_width;
                    encoder_factory_client_h = (int)client_height;
                    encoder_factory_codec_type = (CodecType)client_codec_type;
                    encoder_factory_current_bitrate = current_bitrate;
                    if (encoder == NULL) {
                        // Run on this thread bc we have to wait for it anyway, encoder == NULL
                        multithreaded_encoder_factory(NULL);
                        encoder = encoder_factory_result;
                        pending_encoder = false;
                        update_encoder = false;

                        new_encoder_used = true;
                    } else {
                        fractal_create_thread(multithreaded_encoder_factory,
                                              "multithreaded_encoder_factory", NULL);
                        pending_encoder = true;
                    }
                }

                // Reinitializes the internal context that handles transferring from device to
                // encoder.
                if (new_encoder_used) {
                    reinitialize_transfer_context(device, encoder);
                }
            }
        }

        // SENDING LOGIC:
        // first, we call capture_screen, which returns how many frames have passed since the last
        // call to capture_screen, If we are using Nvidia, the captured frame is also
        // hardware-encoded. Otherwise, we pass the most recent frame to our encoder and call
        // video_encoder_encode_frame on it. Then, we send encoded frames to the client. If the
        // frames we encode + send are a subset of the frames we capture, that means we are dropping
        // frames, which is suboptimal and should be investigated.

        // Accumulated_frames is equal to how many frames have passed since the
        // last call to CaptureScreen
        int accumulated_frames = 0;
        if (get_timer(last_frame_capture) > 1.0 / FPS) {
            accumulated_frames = capture_screen(device);
            // LOG_INFO( "CaptureScreen: %d", accumulated_frames );
        }

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            LOG_WARNING("Failed to capture screen");

            destroy_capture_device(device);
            device = NULL;
            update_device = true;

            fractal_sleep(100);
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
            // If 1/MIN_FPS has passed, but no accumulated_frames have happened,
            // Then we just resend the current frame
            if (accumulated_frames == 0) {
                // LOG_INFO("Resending current frame!");
            }

            // transfer the capture of the latest frame from the device to the encoder
            // This function will DXGI CUDA optimize if possible,
            // Or do nothing if the device already encoded the capture
            // with nvidia capture SDK
            if (transfer_capture(device, encoder) != 0) {
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

            // LOG_INFO("Encode Time: %f (%d) (%d)", get_timer(t),
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

                    // LOG_INFO("Size: %d, MBPS: %f, VS MAX MBPS: %f, Time:
                    // %f, Transmit Time: %f, Delay: %f",
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

                if (encoder->encoded_frame_size > (int)MAX_FRAME_VIDEODATA_SIZE) {
                    LOG_ERROR("Frame videodata too large: %d", encoder->encoded_frame_size);
                } else {
                    // Create frame struct with compressed frame data and
                    // metadata
                    static char buf[LARGEST_FRAME_SIZE];
                    VideoFrame* frame = (VideoFrame*)buf;
                    frame->width = encoder->out_width;
                    frame->height = encoder->out_height;
                    frame->codec_type = encoder->codec_type;
                    if (encoder->encoded_frame_size != 0) {
                        memcpy(frame->metrics_ids, device->nvidia_capture_device.metrics_ids,
                               sizeof(frame->metrics_ids));
                    }

                    static FractalCursorImage cursor_cache[2];
                    static int last_cursor_id = 0;
                    int current_cursor_id = (last_cursor_id + 1) % 2;

                    FractalCursorImage* last_cursor = &cursor_cache[last_cursor_id];
                    FractalCursorImage* current_cursor = &cursor_cache[current_cursor_id];

                    get_current_cursor(current_cursor);

                    // If the current cursor is the same as the last cursor,
                    // just don't send any cursor
                    if (memcmp(last_cursor, current_cursor, sizeof(FractalCursorImage)) == 0) {
                        set_frame_cursor_image(frame, NULL);
                    } else {
                        set_frame_cursor_image(frame, current_cursor);
                    }

                    last_cursor_id = current_cursor_id;

                    // frame is an iframe if this frame does not require previous frames to
                    // render
                    frame->is_iframe = encoder->is_iframe;

                    frame->videodata_length = encoder->encoded_frame_size;

                    write_packets_to_buffer(encoder->num_packets, encoder->packets,
                                            (void*)get_frame_videodata(frame));

                    // LOG_INFO("Sent video packet %d (Size: %d) %s", id,
                    // encoder->encoded_frame_size, frame->is_iframe ?
                    // "(I-frame)" :
                    // "");
                    PeerUpdateMessage* peer_update_msgs = get_frame_peer_messages(frame);

                    size_t num_msgs;
                    read_lock(&is_active_rwlock);
                    fractal_lock_mutex(state_lock);

                    if (fill_peer_update_messages(peer_update_msgs, &num_msgs) != 0) {
                        LOG_ERROR("Failed to copy peer update messages.");
                    }
                    frame->num_peer_update_msgs = (int)num_msgs;

                    start_timer(&t);

                    // Send video packet to client
                    if (broadcast_udp_packet(
                            PACKET_VIDEO, (uint8_t*)frame, get_total_frame_size(frame), id,
                            STARTING_BURST_BITRATE, video_buffer[id % VIDEO_BUFFER_SIZE],
                            video_buffer_packet_len[id % VIDEO_BUFFER_SIZE]) != 0) {
                        LOG_WARNING("Could not broadcast video frame ID %d", id);
                    } else {
                        // Only increment ID if the send succeeded
                        id++;
                    }
                    fractal_unlock_mutex(state_lock);
                    read_unlock(&is_active_rwlock);

                    // LOG_INFO( "Send Frame Time: %f, Send Frame Size: %d\n",
                    // get_timer( t ), frame_size );

                    previous_frame_size = encoder->encoded_frame_size;
                    // double server_frame_time =
                    // get_timer(server_frame_timer); LOG_INFO("Server Frame
                    // Time for ID %d: %f", id, server_frame_time);
                }
            }
        }
    }

    destroy_capture_device(device);
    device = NULL;
    multithreaded_destroy_encoder(encoder);
    encoder = NULL;

    return 0;
}
