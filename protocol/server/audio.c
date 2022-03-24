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

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

// Number of audio previous frames that will be resent along with the current frame.
// Resending of audio is done pro-actively as audio packet loss doesn't have enough time for client
// to send nack and recover. Since audio bitrate is just 128Kbps, the extra bandwidth used for
// resending audio packets is still acceptable.
#define NUM_PREV_AUDIO_FRAMES_RESEND 1

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

    // Wait for the client to lock
    int previous_connection_id = -1;
    ClientLock* client_lock = client_active_lock(state->client);

    // Audio Loop
    while (client_lock != NULL) {
        // Refresh the client activation lock, to let the client (re/de)activate if it's trying to
        client_active_unlock(client_lock);
        client_lock = client_active_lock(state->client);
        if (client_lock == NULL) {
            break;
        }

        // If a new connection occured, restart the stream
        if (previous_connection_id != state->client->connection_id) {
            // Clear the audio buffer on a new connection
            for (get_next_packet(audio_device); packet_available(audio_device);
                 get_next_packet(audio_device)) {
                get_buffer(audio_device);
                release_buffer(audio_device);
            }
            previous_connection_id = state->client->connection_id;
        }

        // Process each available audio packet
        for (get_next_packet(audio_device); packet_available(audio_device);
             get_next_packet(audio_device)) {
            get_buffer(audio_device);

            if (audio_device->buffer_size > 10000) {
                LOG_ERROR("Audio buffer size too large!");
            } else if (audio_device->buffer_size > 0) {
                // Add samples to encoder fifo
                audio_encoder_fifo_intake(audio_encoder, audio_device->buffer,
                                          audio_device->frames_available);

                // While fifo has enough samples for an aac frame, handle it
                while (av_audio_fifo_size(audio_encoder->audio_fifo) >=
                       audio_encoder->context->frame_size) {
                    // Create and encode a frame

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
                    static char buf[LARGEST_AUDIOFRAME_SIZE];
                    AudioFrame* frame = (AudioFrame*)buf;
                    frame->audio_frequency = audio_device->sample_rate;

                    int data_len = write_avpackets_to_buffer(
                        (void*)frame->data, MAX_AUDIOFRAME_DATA_SIZE, audio_encoder->packets,
                        audio_encoder->num_packets);
                    if (data_len == -1) {
                        LOG_ERROR("Audio data too large for: %d", MAX_AUDIOFRAME_DATA_SIZE);
                    } else {
                        frame->data_length = data_len;

                        send_packet(&state->client->udp_context, PACKET_AUDIO, frame,
                                    MAX_AUDIOFRAME_METADATA_SIZE + data_len, id, false);
                        // Simulate nacks to trigger re-sending of previous frames.
                        // TODO: Move into udp.c
                        udp_reset_duplicate_packet_counter(&state->client->udp_context,
                                                           PACKET_AUDIO);
                        for (int i = 1; i <= NUM_PREV_AUDIO_FRAMES_RESEND && id - i > 0; i++) {
                            // Audio is always only one UDP packet per audio frame.
                            // Average bytes per audio frame = (Samples_per_frame * Bitrate) /
                            //                                 (BITS_IN_BYTE * Sampling freq)
                            //                               = (480 * 128000) / (8 * 48000)
                            //                               = 160 bytes only
                            udp_resend_packet(&state->client->udp_context, PACKET_AUDIO, id - i, 0);
                        }
                        id++;
                    }
                }
            }

            release_buffer(audio_device);
        }
        wait_timer(audio_device);
    }

    // If we're holding a client lock, unlock it
    if (client_lock != NULL) {
        client_active_unlock(client_lock);
    }

    // TODO: Why are we no longer destroying this?
    // destroy_audio_encoder(audio_encoder);
    destroy_audio_device(audio_device);
    return 0;
}
