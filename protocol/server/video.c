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

extern volatile bool stop_streaming;
extern volatile bool wants_iframe;
extern volatile bool update_encoder;

static bool pending_encoder;
static bool encoder_finished;
static VideoEncoder* encoder_factory_result = NULL;

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
    if (encoder_factory_result == NULL) {
        LOG_FATAL("Could not create an encoder, giving up!");
    }
    encoder_finished = true;
    return 0;
}

int32_t multithreaded_destroy_encoder(void* opaque) {
    VideoEncoder* encoder = (VideoEncoder*)opaque;
    destroy_video_encoder(encoder);
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
    bool transfer_context_active = false;

    while (!exiting) {
        if (num_active_clients == 0 || client_width < 0 || client_height < 0 || client_dpi < 0) {
            fractal_sleep(5);
            continue;
        }

        // Update device with new parameters

        // YUV pixel format requires the width to be a multiple of 4 and the height to be a
        // multiple of 2 (see `bRoundFrameSize` in NvFBC.h). By default, the dimensions will be
        // implicitly rounded up, but for some reason it looks better if we explicitly set the
        // size. Also for some reason it actually rounds the width to a multiple of 8.
        int true_width = client_width + 7 - ((client_width + 7) % 8);
        int true_height = client_height + 1 - ((client_height + 1) % 2);

        // If we got an update device request, we should update the device
        if (update_device) {
            update_device = false;

            LOG_INFO("Received an update capture device request to dimensions %dx%d with DPI %d",
                     true_width, true_height, client_dpi);

            // If a device already exists, we should reconfigure or destroy it
            if (device != NULL) {
                if (transfer_context_active) {
                    close_transfer_context(device, encoder);
                    transfer_context_active = false;
                    encoder = NULL;
                }

                // Eh hacky kill bc reconfigure stops the cuda ctx
                if (encoder) {
                    LOG_INFO("Destroying encoder...");
                    destroy_video_encoder(encoder);
                    encoder = NULL;
                    update_encoder = true;
                }

                if (reconfigure_capture_device(device, true_width, true_height, client_dpi)) {
                    // Reconfigured the capture device!
                    // No need to recreate it, the device has now been updated
                    LOG_INFO("Successfully reconfigured the capture device");
                    // We should also update the encoder since the device has been reconfigured
                    update_encoder = true;
                } else {
                    // Destroying the old capture device so that a new one can be recreated below
                    LOG_FATAL(
                        "Failed to reconfigure the capture device! We probably have a memory "
                        "leak!");
                    // "Destroying and recreating the capture device instead!");

                    // For the time being, we have disabled the reconfigure functionality because
                    // of some weirdness happening in vkCreateDevice()
                }
            } else {
                LOG_INFO("No capture device exists yet, creating a new one.");
            }
        }

        // If no device is set, we need to create one
        if (device == NULL) {
            device = &rdevice;
            if (create_capture_device(device, true_width, true_height, client_dpi) < 0) {
                LOG_WARNING("Failed to create capture device");
                device = NULL;
                update_device = true;

                fractal_sleep(100);
                continue;
            }

            LOG_INFO("Created a new Capture Device of dimensions %dx%d with DPI %d", device->width,
                     device->height, client_dpi);

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

            // Next, we should update our ffmpeg encoder
            update_encoder = true;
        }

        // Update encoder with new parameters
        if (update_encoder) {
            // If this is a new update encoder request, log it
            if (!pending_encoder) {
                LOG_INFO("Update encoder request received, will update the encoder now!");
            }

            /*
            // First, try to simply reconfigure the encoder to
            // handle the update_encoder event
            if (encoder != NULL) {
                if (reconfigure_encoder(encoder, device->width, device->height,
                                        (int)(max_mbps * 1024 * 1024),
                                        (CodecType)client_codec_type)) {
                    // If we could update the encoder in-place, then we're done updating the encoder
                    LOG_INFO(
                        "Reconfigured Encoder to %dx%d using Bitrate: %d from %f, and Codec %d",
                        device->width, device->height, current_bitrate, max_mbps,
                        client_codec_type);
                    current_bitrate = (int)(max_mbps * 1024 * 1024);
                    update_encoder = false;
                } else {
                    // TODO: Make LOG_ERROR after ffmpeg reconfiguration is implemented
                    LOG_INFO("Reconfiguration failed! Creating a new encoder!");
                }
            }
            */

            // If reconfiguration didn't happen, we still need to update the encoder
            if (update_encoder) {
                // Otherwise, this capture device must use an external encoder,
                // so we should start making it in our encoder factory
                if (pending_encoder) {
                    if (encoder_finished) {
                        // Once encoder_finished, we'll destroy the old one that we've been using,
                        // and replace it with the result of multithreaded_encoder_factory
                        if (encoder) {
                            if (transfer_context_active) {
                                close_transfer_context(device, encoder);
                                transfer_context_active = false;
                            }
                            fractal_create_thread(multithreaded_destroy_encoder,
                                                  "multithreaded_destroy_encoder", encoder);
                        }
                        encoder = encoder_factory_result;
                        pending_encoder = false;
                        update_encoder = false;
                    }
                } else {
                    // Starting making new encoder. This will set pending_encoder=true, but won't
                    // actually update it yet, we'll still use the old one for a bit
                    LOG_INFO(
                        "Creating a new Encoder of dimensions %dx%d using Bitrate: %d from %f, and "
                        "Codec %d",
                        device->width, device->height, current_bitrate, max_mbps,
                        (int)client_codec_type);
                    current_bitrate = (int)(max_mbps * 1024 * 1024);
                    encoder_finished = false;
                    encoder_factory_server_w = device->width;
                    encoder_factory_server_h = device->height;
                    encoder_factory_client_w = (int)client_width;
                    encoder_factory_client_h = (int)client_height;
                    encoder_factory_codec_type = (CodecType)client_codec_type;
                    encoder_factory_current_bitrate = current_bitrate;

                    // If using nvidia, then we must destroy the existing encoder first
                    // We can't have two nvidia encoders active or the 2nd attempt to
                    // create one will fail
                    // If using ffmpeg, if the dimensions don't match, then we also need to destroy
                    // the old encoder, since we'll no longer be able to pass captured frames into
                    // it
                    // For now, we'll just always destroy the encoder right here
                    /*
                    if (encoder != NULL) {
                        if (transfer_context_active) {
                            close_transfer_context(device, encoder);
                            transfer_context_active = false;
                        }
                        destroy_video_encoder(encoder);
                        encoder = NULL;
                    }
                    */
                    if (encoder == NULL) {
                        // Run on this thread bc we have to wait for it anyway since encoder == NULL
                        multithreaded_encoder_factory(NULL);
                        encoder = encoder_factory_result;
                        pending_encoder = false;
                        update_encoder = false;
                    } else {
                        fractal_create_thread(multithreaded_encoder_factory,
                                              "multithreaded_encoder_factory", NULL);
                        pending_encoder = true;
                    }
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
        if (get_timer(last_frame_capture) > 1.0 / FPS && (!stop_streaming || wants_iframe)) {
            accumulated_frames = capture_screen(device);
            // LOG_INFO( "CaptureScreen: %d", accumulated_frames );
        }

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            LOG_WARNING("Failed to capture screen");
            transfer_context_active = false;
            close_transfer_context(device, encoder);
            destroy_capture_device(device);
            device = NULL;
            update_device = true;

            fractal_sleep(100);
            continue;
        }

        if (!transfer_context_active) {
            start_transfer_context(device, encoder);
            transfer_context_active = true;
        }

        clock server_frame_timer;
        start_timer(&server_frame_timer);

        // Only if we have a frame to render or we need to send something to keep up with MIN_FPS
        if (accumulated_frames > 0 || wants_iframe ||
            get_timer(last_frame_capture) > 1.0 / MIN_FPS) {
            // LOG_INFO("Frame Time: %f\n", get_timer(last_frame_capture));

            start_timer(&last_frame_capture);

            if (accumulated_frames > 1) {
                LOG_INFO("Accumulated Frames: %d", accumulated_frames);
            }
            // If 1/MIN_FPS has passed but no accumulated_frames have happened (or the client asked
            // the server to stop encoding frames to save resources via `stop_streaming`), then we
            // skip everything in here, and we just send an empty frame with is_empty_frame = true.
            // NOTE: `accumulated_frames` is the number of new frames collected since the last frame
            // sent. If this is 0, then this frame is just a repeat of the frame before it (which
            // we're sending to keep the framerate above MIN_FPS).
            // ADDITIONAL NOTE: If wants_iframe gets set to true when stop_streaming is true or
            // accumulated_frames is 0 (which it ordinarily shouldn't), we HAVE TO render that frame
            // or the server will spazz out and start sending 1000's of FPS.
            if (accumulated_frames > 0 || wants_iframe) {
                // transfer the capture of the latest frame from the device to
                // the encoder,
                // This function will try to CUDA/OpenGL optimize the transfer by
                // only passing a GPU reference rather than copy to/from the CPU
                if (transfer_capture(device, encoder) != 0) {
                    // if there was a failure
                    LOG_ERROR("transfer_capture failed! Exiting!");
                    exiting = true;
                    break;
                }

                if (wants_iframe) {
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
                                                      BYTES_IN_KILOBYTE / BYTES_IN_KILOBYTE /
                                                      max_mbps;
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

                    if (encoder->encoded_frame_size > (int)MAX_VIDEOFRAME_DATA_SIZE) {
                        LOG_ERROR("Frame videodata too large: %d", encoder->encoded_frame_size);
                    } else {
                        // Create frame struct with compressed frame data and
                        // metadata
                        static char buf[LARGEST_VIDEOFRAME_SIZE];
                        VideoFrame* frame = (VideoFrame*)buf;
                        frame->width = encoder->out_width;
                        frame->height = encoder->out_height;
                        frame->codec_type = encoder->codec_type;
                        frame->is_empty_frame = false;

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

#if LOG_VIDEO
                        LOG_INFO("Sent video packet %d (Size: %d) %s", id,
                                 encoder->encoded_frame_size, frame->is_iframe ? "(I-frame)" : "");
#endif  // LOG_VIDEO

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
            } else {
                // If we don't have a new frame to send, let's just send an empty one
                static char mini_buf[sizeof(VideoFrame)];
                VideoFrame* frame = (VideoFrame*)mini_buf;
                frame->is_empty_frame = true;
                // This signals that the screen hasn't changed, so don't bother rendering
                // this frame and just keep showing the last one.
                // We don't need to fill out the rest of the fields of the VideoFrame because
                // is_empty_frame is true, so it will just be ignored by the client.

                if (broadcast_udp_packet(PACKET_VIDEO, (uint8_t*)frame, sizeof(VideoFrame), id,
                                         STARTING_BURST_BITRATE,
                                         video_buffer[id % VIDEO_BUFFER_SIZE],
                                         video_buffer_packet_len[id % VIDEO_BUFFER_SIZE]) != 0) {
                    LOG_WARNING("Could not broadcast video frame ID %d", id);
                } else {
                    // Only increment ID if the send succeeded
                    id++;
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
