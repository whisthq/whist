/**
 * Copyright 2021 Whist Technologies, Inc.
 * @file audio.c
 * @brief This file contains all code that interacts directly with processing
 *        audio on the server.
============================
Usage
============================

multithreaded_send_audio() is called on its own thread and loops repeatedly to capture and send
audio.
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

#include <fractal/audio/audiocapture.h>
#include <fractal/audio/audioencode.h>
#include <fractal/logging/log_statistic.h>
#include <fractal/utils/avpacket_buffer.h>
#include "client.h"
#include "network.h"
#include "audio.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

extern volatile bool exiting;

extern int sample_rate;

/*
============================
Public Function Implementations
============================
*/

int32_t multithreaded_send_audio(void* opaque) {
    UNUSED(opaque);
    fractal_set_thread_priority(WHIST_THREAD_PRIORITY_REALTIME);
    int id = 1;

    AudioDevice* audio_device = create_audio_device();
    if (!audio_device) {
        LOG_ERROR("Failed to create audio device...");
        return -1;
    }
    LOG_INFO("Created audio device!");
    start_audio_device(audio_device);
    AudioEncoder* audio_encoder = create_audio_encoder(AUDIO_BITRATE, audio_device->sample_rate);
    if (!audio_encoder) {
        LOG_ERROR("Failed to create audio encoder...");
        return -1;
    }

    int res;
    // Tell the client what audio frequency we're using
    sample_rate = audio_device->sample_rate;
    LOG_INFO("Audio Frequency: %d", audio_device->sample_rate);

    add_thread_to_client_active_dependents();

    // setup
    bool assuming_client_active = false;
    while (!exiting) {
        update_client_active_status(&assuming_client_active);

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
                while (av_audio_fifo_size(audio_encoder->audio_fifo) >=
                       audio_encoder->context->frame_size) {
                    // create and encode a frame

                    clock t;
                    start_timer(&t);
                    res = audio_encoder_encode_frame(audio_encoder);

                    if (res < 0) {
                        // bad boy error
                        LOG_WARNING("error encoding packet");
                        continue;
                    } else if (res > 0) {
                        // no data or need more data
                        break;
                    }
                    log_double_statistic("Audio encode time (ms)", get_timer(t) * 1000);
                    if (audio_encoder->encoded_frame_size > (int)MAX_AUDIOFRAME_DATA_SIZE) {
                        LOG_ERROR("Audio data too large: %d", audio_encoder->encoded_frame_size);
                    } else if (assuming_client_active && client.is_active) {
                        static char buf[LARGEST_AUDIOFRAME_SIZE];
                        AudioFrame* frame = (AudioFrame*)buf;
                        frame->data_length = audio_encoder->encoded_frame_size;

                        write_avpackets_to_buffer(audio_encoder->num_packets,
                                                  audio_encoder->packets, (void*)frame->data);

                        if (client.is_active) {
                            send_packet(&client.udp_context, PACKET_AUDIO, frame,
                                        audio_encoder->encoded_frame_size + sizeof(int), id);
                            id++;
                        }
                    }
                }
#else
                if (client.is_active) {
                    send_packet(&client.udp_context, PACKET_AUDIO, audio_device->buffer,
                                audio_device->buffer_size, id);
                    id++;
                }
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
