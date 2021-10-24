/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
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
#include <fractal/logging/log_statistic.h>
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

extern volatile int max_bitrate;
volatile int client_width = -1;
volatile int client_height = -1;
volatile int client_dpi = -1;
volatile CodecType client_codec_type = CODEC_TYPE_UNKNOWN;
volatile bool update_device = true;

#define VIDEO_BUFFER_SIZE 25
#define MAX_NUM_VIDEO_INDICES 500
FractalPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_NUM_VIDEO_INDICES];

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
static int encoder_factory_bitrate;
static CodecType encoder_factory_codec_type;

static bool transfer_context_active;

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
        encoder_factory_client_h, encoder_factory_bitrate, encoder_factory_codec_type);
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

/**
 * @brief                   Creates a new CaptureDevice
 *
 * @param statistics_timer  Pointer to statistics timer used for logging
 * @param device            CaptureDevice pointer
 * @param rdevice           CaptureDevice pointer
 * @param encoder           VideoEncoder pointer
 * @param true_width        True width of client screen
 * @param true_height       True height of client screen
 * @return                  On success, 0. On failure, -1.
 */
int32_t create_new_device(clock* statistics_timer, CaptureDevice** device, CaptureDevice* rdevice,
                          VideoEncoder** encoder, uint32_t true_width, uint32_t true_height) {
    start_timer(statistics_timer);
    *device = rdevice;
    if (create_capture_device(*device, true_width, true_height, client_dpi) < 0) {
        LOG_WARNING("Failed to create capture device");
        *device = NULL;
        update_device = true;

        fractal_sleep(100);
        return -1;
    }

    LOG_INFO("Created a new Capture Device of dimensions %dx%d with DPI %d", (*device)->width,
             (*device)->height, client_dpi);

    // If an encoder is pending, while capture_device is updating, then we should wait
    // for it to be created
    while (pending_encoder) {
        if (encoder_finished) {
            *encoder = encoder_factory_result;
            pending_encoder = false;
            break;
        }
        fractal_sleep(1);
    }

    // Next, we should update our ffmpeg encoder
    update_encoder = true;

    log_double_statistic("Create capture device time (ms)",
                         get_timer(*statistics_timer) * MS_IN_SECOND);

    return 0;
}

/**
 * @brief                           Sends the populated video frames to the
 *                                  client
 *
 * @param statistics_timer          Pointer to statistics timer used for logging
 * @param server_frame_timer        Pointer to server_frame_timer
 * @param device                    CaptureDevice pointer
 * @param encoder                   VideoEncoder pointer
 * @param id                        Pointer to frame id
 */
void send_populated_frames(clock* statistics_timer, clock* server_frame_timer,
                           CaptureDevice* device, VideoEncoder* encoder, int* id) {
    // transfer the capture of the latest frame from the device to
    // the encoder,
    // This function will try to CUDA/OpenGL optimize the transfer by
    // only passing a GPU reference rather than copy to/from the CPU

    // Create frame struct with compressed frame data and
    // metadata
    static char buf[LARGEST_VIDEOFRAME_SIZE];
    VideoFrame* frame = (VideoFrame*)buf;
    frame->width = encoder->out_width;
    frame->height = encoder->out_height;
    frame->codec_type = encoder->codec_type;
    frame->is_empty_frame = false;
    frame->is_window_visible = true;
    frame->corner_color = device->corner_color;

    static FractalCursorImage cursor_cache[2];
    static int last_cursor_id = 0;
    int current_cursor_id = (last_cursor_id + 1) % 2;

    FractalCursorImage* last_cursor = &cursor_cache[last_cursor_id];
    FractalCursorImage* current_cursor = &cursor_cache[current_cursor_id];

    start_timer(statistics_timer);
    get_current_cursor(current_cursor);
    log_double_statistic("get_current_cursor time (ms)",
                         get_timer(*statistics_timer) * MS_IN_SECOND);

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

    write_avpackets_to_buffer(encoder->num_packets, encoder->packets,
                              (void*)get_frame_videodata(frame));

#if LOG_VIDEO
    LOG_INFO("Sent video packet %d (Size: %d) %s", *id, encoder->encoded_frame_size,
             frame->is_iframe ? "(I-frame)" : "");
#endif  // LOG_VIDEO
    start_timer(statistics_timer);

    // Packetize the frame
    int num_packets =
        write_payload_to_packets((uint8_t*)frame, get_total_frame_size(frame), *id, PACKET_VIDEO,
                                 video_buffer[(*id) % VIDEO_BUFFER_SIZE], MAX_NUM_VIDEO_INDICES);

    if (num_packets < 0) {
        // Couldn't send frame, flush with IDR
        LOG_ERROR("Failed to write video packet to buffer");
        wants_iframe = true;
    } else {
        // Send the packets to the client
        for (int i = 0; i < num_packets; ++i) {
            if (broadcast_udp_packet(&video_buffer[(*id) % VIDEO_BUFFER_SIZE][i],
                                     get_packet_size(&video_buffer[(*id) % VIDEO_BUFFER_SIZE][i])) <
                0) {
                LOG_WARNING("Failed to broadcast video packet: id %d, index %d", *id, i);
            }
        }
// (Disable this logic for now).
#define MOD 0
        for (int j = 0; j < MOD; ++j) {
            for (int i = j; i < num_packets; i += MOD) {
                if (broadcast_udp_packet(
                        &video_buffer[(*id) % VIDEO_BUFFER_SIZE][i],
                        get_packet_size(&video_buffer[(*id) % VIDEO_BUFFER_SIZE][i])) < 0) {
                    LOG_WARNING("Failed to broadcast video packet: id %d, index %d", (*id), i);
                }
            }
        }
        // Some subset of the frame may have been sent,
        // so we have to increment ID to prevent overlap
        (*id)++;
    }
}

/**
 * @brief           Attempts to capture the screen. Afterwards sets update_device
 *                  to true
 *
 * @param device    CaptureDevice pointer
 * @param encoder   VideoEncoder pointer
 */
void retry_capture_screen(CaptureDevice* device, VideoEncoder* encoder) {
    LOG_WARNING("Failed to capture screen");
    transfer_context_active = false;
    close_transfer_context(device, encoder);
    // The Nvidia Encoder must be wrapped in the lifetime of the capture device
    if (encoder != NULL && encoder->active_encoder == NVIDIA_ENCODER) {
        multithreaded_destroy_encoder(encoder);
        encoder = NULL;
    }
    destroy_capture_device(device);
    device = NULL;
    update_device = true;

    fractal_sleep(100);
}

/**
 * @brief                  Updates dimensions of capture device and sets
 *                         update_encoder to true
 *
 * @param statistics_timer Pointer to the timer used for statistics logging
 * @param device           CaptureDevice pointer
 * @param encoder          VideoEncoder pointer
 * @param true_width       True width of client screen
 * @param true_height      True height of client screen
 */
void update_current_device(clock* statistics_timer, CaptureDevice* device, VideoEncoder* encoder,
                           uint32_t true_width, uint32_t true_height) {
    update_device = false;
    start_timer(statistics_timer);

    LOG_INFO("Received an update capture device request to dimensions %dx%d with DPI %d",
             true_width, true_height, client_dpi);

    // If a device already exists, we should reconfigure or destroy it
    if (device != NULL) {
        if (transfer_context_active) {
            close_transfer_context(device, encoder);
            transfer_context_active = false;
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
    log_double_statistic("Update capture device time (ms)",
                         get_timer(*statistics_timer) * MS_IN_SECOND);
}

/**
 * @brief                         Sends an empty frame to the client
 *
 * @param id                      Pointer to the frame id
 */
void send_empty_frame(int* id) {
    // If we don't have a new frame to send, let's just send an empty one
    static char mini_buf[sizeof(VideoFrame)];
    VideoFrame* frame = (VideoFrame*)mini_buf;
    frame->is_empty_frame = true;
    // This signals that the screen hasn't changed, so don't bother rendering
    // this frame and just keep showing the last one.
    frame->is_window_visible = !stop_streaming;
    // We don't need to fill out the rest of the fields of the VideoFrame because
    // is_empty_frame is true, so it will just be ignored by the client.

    int num_packets =
        write_payload_to_packets((uint8_t*)frame, sizeof(VideoFrame), *id, PACKET_VIDEO,
                                 video_buffer[(*id) % VIDEO_BUFFER_SIZE], MAX_NUM_VIDEO_INDICES);

    if (num_packets < 0) {
        // We don't need to set wants_iframe, since a frame was never encoded
        LOG_ERROR("Failed to write video packet to buffer");
    } else {
        for (int i = 0; i < num_packets; ++i) {
            if (broadcast_udp_packet(&video_buffer[(*id) % VIDEO_BUFFER_SIZE][i],
                                     get_packet_size(&video_buffer[(*id) % VIDEO_BUFFER_SIZE][i])) <
                0) {
                LOG_WARNING("Failed to broadcast video packet: id %d, index %d", id, i);
            }
        }
        // We may have sent some packets,
        // so we must increment the ID to prevent overlap
        (*id)++;
    }
}

/**
 * @brief           Updates the encoder upon request. Note that this function
 *                  _returns_ the updated encoder, due to the encoder factory.
 *
 * @param encoder   VideoEncoder pointer
 *
 * @param device    CaptureDevice pointer
 *
 * @returns         The new encoder
 */
VideoEncoder* do_update_encoder(VideoEncoder* encoder, CaptureDevice* device) {
    // If this is a new update encoder request, log it
    if (!pending_encoder) {
        LOG_INFO("Update encoder request received, will update the encoder now!");
    }

    // First, try to simply reconfigure the encoder to
    // handle the update_encoder event
    if (encoder != NULL) {
        // store it to prevent race conditions if bitrate is updated
        int new_bitrate = max_bitrate;
        if (reconfigure_encoder(encoder, device->width, device->height, new_bitrate,
                                (CodecType)client_codec_type)) {
            // If we could update the encoder in-place, then we're done updating the encoder
            LOG_INFO("Reconfigured Encoder to %dx%d using Bitrate: %d from %f, and Codec %d",
                     device->width, device->height, new_bitrate, new_bitrate / 1024.0 / 1024.0,
                     client_codec_type);
            update_encoder = false;
        } else {
            // TODO: Make LOG_ERROR after ffmpeg reconfiguration is implemented
            LOG_INFO("Reconfiguration failed! Creating a new encoder!");
        }
    }

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
                    FractalThread encoder_destroy_thread = fractal_create_thread(
                        multithreaded_destroy_encoder, "multithreaded_destroy_encoder", encoder);
                    fractal_detach_thread(encoder_destroy_thread);
                }
                encoder = encoder_factory_result;
                pending_encoder = false;
                update_encoder = false;
            }
        } else {
            // Starting making new encoder. This will set pending_encoder=true, but won't
            // actually update it yet, we'll still use the old one for a bit
            int new_bitrate = max_bitrate;
            LOG_INFO(
                "Creating a new Encoder of dimensions %dx%d using Bitrate: %d from %f, and "
                "Codec %d",
                device->width, device->height, new_bitrate, new_bitrate / 1024.0 / 1024.0,
                (int)client_codec_type);
            encoder_finished = false;
            encoder_factory_server_w = device->width;
            encoder_factory_server_h = device->height;
            encoder_factory_client_w = (int)client_width;
            encoder_factory_client_h = (int)client_height;
            encoder_factory_codec_type = (CodecType)client_codec_type;
            encoder_factory_bitrate = new_bitrate;

            // If using nvidia, then we must destroy the existing encoder first
            // We can't have two nvidia encoders active or the 2nd attempt to
            // create one will fail
            // If using ffmpeg, if the dimensions don't match, then we also need to destroy
            // the old encoder, since we'll no longer be able to pass captured frames into
            // it
            // For now, we'll just always destroy the encoder right here
            if (encoder != NULL && encoder->active_encoder == NVIDIA_ENCODER) {
                if (transfer_context_active) {
                    close_transfer_context(device, encoder);
                    transfer_context_active = false;
                }
                destroy_video_encoder(encoder);
                encoder = NULL;
            }

            if (encoder == NULL) {
                // Run on this thread bc we have to wait for it anyway since encoder == NULL
                multithreaded_encoder_factory(NULL);
                encoder = encoder_factory_result;
                pending_encoder = false;
                update_encoder = false;
            } else {
                FractalThread encoder_creator_thread = fractal_create_thread(
                    multithreaded_encoder_factory, "multithreaded_encoder_factory", NULL);
                fractal_detach_thread(encoder_creator_thread);
                pending_encoder = true;
            }
        }
    }
    return encoder;
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

    VideoEncoder* encoder = NULL;

    clock world_timer;
    start_timer(&world_timer);

    clock statistics_timer;

    int id = 1;
    update_device = true;

    clock last_frame_capture;
    start_timer(&last_frame_capture);

    pending_encoder = false;
    encoder_finished = false;
    transfer_context_active = false;

    add_thread_to_client_active_dependents();

    int consecutive_identical_frames = 0;

    bool assuming_client_active = false;
    while (!exiting) {
        update_client_active_status(&assuming_client_active);

        if (!assuming_client_active) {
            fractal_sleep(1);
            continue;
        }

        static clock send_video_loop_timer;
        start_timer(&send_video_loop_timer);
        if (client_width < 0 || client_height < 0 || client_dpi < 0) {
            // if we've just started, capture at a common width, height, and DPI
            // when a client connects, they'll request a dimension change to the correct dimensions
            // + DPI. The height, width, and DPI all match the default on a 2020 M1 MacBook Pro.
            // A cross-platform default DPI would be 192; width and height depend on things like
            // the Windows Start Bar, the macOS Dock and Menu Bar, and window controls.
            client_width = 2880;
            client_height = 1524;
            client_dpi = 192;
            client_codec_type = CODEC_TYPE_H264;
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
            update_current_device(&statistics_timer, device, encoder, true_width, true_height);
        }

        // If no device is set, we need to create one
        if (device == NULL) {
            if (create_new_device(&statistics_timer, &device, &rdevice, &encoder, true_width,
                                  true_height) < 0) {
                continue;
            }
        }

        // Update encoder with new parameters
        if (update_encoder) {
            start_timer(&statistics_timer);
            encoder = do_update_encoder(encoder, device);
            log_double_statistic("Update encoder time (ms)",
                                 get_timer(statistics_timer) * MS_IN_SECOND);
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
            start_timer(&statistics_timer);
            accumulated_frames = capture_screen(device);
            log_double_statistic("Capture screen time (ms)",
                                 get_timer(statistics_timer) * MS_IN_SECOND);
#if LOG_VIDEO
            if (accumulated_frames > 1) {
                LOG_INFO("Missed Frames! %d frames passed since last capture", accumulated_frames);
            }
#endif
            // LOG_INFO( "CaptureScreen: %d", accumulated_frames );
        }

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            retry_capture_screen(device, encoder);
            continue;
        }

        if (!transfer_context_active) {
            start_transfer_context(device, encoder);
            transfer_context_active = true;
        }

        clock server_frame_timer;
        start_timer(&server_frame_timer);

        // Immediately bring consecutives to 0
        if (accumulated_frames > 0) {
            consecutive_identical_frames = 0;
        }
        // Disable the encoder when we've sent enough identical frames,
        // And no iframe is being requested at this time.
        // When the encoder is disabled, we only wake the client CPU,
        // DISABLED_ENCODER_FPS times per second for just a usec
        bool disable_encoder =
            consecutive_identical_frames > CONSECUTIVE_IDENTICAL_FRAMES && !wants_iframe;
        // Drop the min_fps to DISABLED_ENCODER_FPS when the encoder is disabled
        int min_fps = disable_encoder ? DISABLED_ENCODER_FPS : MIN_FPS;

        // This outer loop potentially runs 10s of thousands of times per second, every ~1usec

        // Only if we have a real frame to send, or we need to keep up with min_fps
        if (accumulated_frames > 0 || wants_iframe ||
            get_timer(last_frame_capture) > 1.0 / min_fps) {
            // This loop only runs ~1/current_fps times per second, every 16-100ms
            // LOG_INFO("Frame Time: %f\n", get_timer(last_frame_capture));
            start_timer(&last_frame_capture);

            if (accumulated_frames == 0) {
                // Slowly increment while receiving identical frames
                consecutive_identical_frames++;
            }
            if (accumulated_frames > 1) {
                LOG_INFO("Accumulated Frames: %d", accumulated_frames);
            }
            if (disable_encoder) {
                // Send an empty frame
                send_empty_frame(&id);
            } else {
                // transfer the capture of the latest frame from the device to
                // the encoder,
                // This function will try to CUDA/OpenGL optimize the transfer by
                // only passing a GPU reference rather than copy to/from the CPU
                start_timer(&statistics_timer);
                if (transfer_capture(device, encoder) != 0) {
                    // if there was a failure
                    LOG_ERROR("transfer_capture failed! Exiting!");
                    exiting = true;
                    break;
                }
                log_double_statistic("Transfer capture time (ms)",
                                     get_timer(statistics_timer) * MS_IN_SECOND);

                if (wants_iframe) {
                    video_encoder_set_iframe(encoder);
                    wants_iframe = false;
                }

                start_timer(&statistics_timer);

                int res = video_encoder_encode(encoder);
                if (res < 0) {
                    // bad boy error
                    LOG_ERROR("Error encoding video frame!");
                    exiting = true;
                    break;
                } else if (res > 0) {
                    // filter graph is empty
                    LOG_ERROR("video_encoder_encode filter graph failed! Exiting!");
                    exiting = true;
                    break;
                }
                log_double_statistic("Video encode time (ms)",
                                     get_timer(statistics_timer) * MS_IN_SECOND);

                if (encoder->encoded_frame_size != 0) {
                    // == Avoid Corruption Note ==
                    // Since the frame made it through the encoder at this point, EITHER
                    // we must increment id++, OR we must set wants_iframe=true and continue
                    // By doing id++, we're telling the decoder that it needs this frame first,
                    // By doing wants_iframe=true continue, the next frame is an iframe anyway
                    // Additionally, if we may have sent any packets, we MUST increment the id
                    // Otherwise, the client may receive packets from two frames with the same id
                    // == Avoid Corruption End Note ==
                    if (encoder->encoded_frame_size > (int)MAX_VIDEOFRAME_DATA_SIZE) {
                        LOG_ERROR("Frame videodata too large: %d", encoder->encoded_frame_size);
                        // This frame is too large to send, but it's an encoder reference frame
                        // So, we'll IDR to flush it out
                        wants_iframe = true;
                        continue;
                    } else {
                        send_populated_frames(&statistics_timer, &server_frame_timer, device,
                                              encoder, &id);

                        log_double_statistic("Video frame send time (ms)",
                                             get_timer(statistics_timer) * MS_IN_SECOND);
                        log_double_statistic("Video frame size", encoder->encoded_frame_size);
                        log_double_statistic("Video frame processing time (ms)",
                                             get_timer(server_frame_timer) * 1000);
                    }
                }
            }
        }
    }

    // The Nvidia Encoder must be wrapped in the lifetime of the capture device,
    // So we destroy the encoder first
    multithreaded_destroy_encoder(encoder);
    encoder = NULL;
    destroy_capture_device(device);
    device = NULL;

    return 0;
}
