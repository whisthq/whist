/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
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

#include <whist/audio/audiocapture.h>
#include <whist/audio/audioencode.h>
#include <whist/logging/log_statistic.h>
#include <whist/utils/avpacket_buffer.h>
#include "client.h"
#include "network.h"
#include "audio.h"
#include "state.h"
#include "server_statistic.h"

// Number of audio previous frames that will be resent along with the current frame.
// Resending of audio is done pro-actively as audio packet loss doesn't have enough time for client
// to send nack and recover. Since audio bitrate is just 128Kbps, the extra bandwidth used for
// resending audio packets is still acceptable.
#define NUM_PREVIOUS_FRAMES_RESEND 2

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

/*
============================
Public Function Implementations
============================
*/

int32_t multithreaded_send_audio(void* opaque) {
    whist_server_state* state = (whist_server_state*)opaque;
    int id = 1;

    whist_set_thread_priority(WHIST_THREAD_PRIORITY_REALTIME);

    AudioDevice* audio_device = create_audio_device();
    if (!audio_device) {
        LOG_ERROR("Failed to create audio device...");
        return -1;
    }
    LOG_INFO("Created audio device! (Audio Frequency: %d", audio_device->sample_rate);
    start_audio_device(audio_device);
    AudioEncoder* audio_encoder = create_audio_encoder(AUDIO_BITRATE, audio_device->sample_rate);
    if (!audio_encoder) {
        LOG_ERROR("Failed to create audio encoder...");
        return -1;
    }

    add_thread_to_client_active_dependents();

    // setup
    bool assuming_client_active = false;
    while (!state->exiting) {
        update_client_active_status(&state->client, &assuming_client_active);

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

                    WhistTimer t;
                    start_timer(&t);
                    int res = audio_encoder_encode_frame(audio_encoder);

                    if (res < 0) {
                        // bad boy error
                        LOG_WARNING("error encoding packet");
                        continue;
                    } else if (res > 0) {
                        // no data or need more data
                        break;
                    }

                    log_double_statistic(AUDIO_ENCODE_TIME, get_timer(&t) * 1000);
                    if (audio_encoder->encoded_frame_size > (int)MAX_AUDIOFRAME_DATA_SIZE) {
                        LOG_ERROR("Audio data too large: %d", audio_encoder->encoded_frame_size);
                    } else if (assuming_client_active && state->client.is_active) {
                        static char buf[LARGEST_AUDIOFRAME_SIZE];
                        AudioFrame* frame = (AudioFrame*)buf;
                        frame->audio_frequency = audio_device->sample_rate;
                        frame->data_length = audio_encoder->encoded_frame_size;

                        write_avpackets_to_buffer(audio_encoder->num_packets,
                                                  audio_encoder->packets, (void*)frame->data);

                        if (state->client.is_active) {
                            send_packet(
                                &state->client.udp_context, PACKET_AUDIO, frame,
                                MAX_AUDIOFRAME_METADATA_SIZE + audio_encoder->encoded_frame_size,
                                id, false);
                            // Simulate nacks to trigger re-sending of previous frames.
                            // TODO: Move into udp.c
                            for (int i = 1; i <= NUM_PREVIOUS_FRAMES_RESEND && id - i > 0; i++) {
                                // Audio is always only one UDP packet per audio frame.
                                // Average bytes per audio frame = (Samples_per_frame * Bitrate) /
                                //                                 (BITS_IN_BYTE * Sampling freq)
                                //                               = (480 * 128000) / (8 * 48000)
                                //                               = 160 bytes only
                                udp_resend_packet(&state->client.udp_context, PACKET_AUDIO, id - i,
                                                  0);
                            }
                            id++;
                        }
                    }
                }
#else
                if (state->client.is_active) {
                    send_packet(&state->client.udp_context, PACKET_AUDIO, audio_device->buffer,
                                audio_device->buffer_size, id, false);
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
