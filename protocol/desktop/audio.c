/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audio.c
 * @brief This file contains all code that interacts directly with processing
 *        audio packets on the client.
============================
Usage
============================

initAudio() must be called first before receiving any audio packets.
updateAudio() gets called immediately after to update the client to the server's
audio format.
*/

/*
============================
Includes
============================
*/

#include "audio.h"
#include "network.h"

extern volatile int audio_frequency;
extern bool has_rendered_yet;

// Hold information about audio data as the packets come in
typedef struct AudioPacket {
    int id;
    int size;
    int nacked_for;
    int nacked_amount;
    char data[MAX_PAYLOAD_SIZE];
} AudioPacket;

#define LOG_AUDIO false

#define AUDIO_QUEUE_LOWER_LIMIT 18000
#define AUDIO_QUEUE_UPPER_LIMIT 59000
#define TARGET_AUDIO_QUEUE_LIMIT 28000

#define MAX_NUM_AUDIO_FRAMES 25
#define MAX_NUM_AUDIO_INDICES 3
#define RECV_AUDIO_BUFFER_SIZE (MAX_NUM_AUDIO_FRAMES * MAX_NUM_AUDIO_INDICES)
AudioPacket receiving_audio[RECV_AUDIO_BUFFER_SIZE];

#define SDL_AUDIO_BUFFER_SIZE 1024

/*
============================
Custom Types
============================
*/

typedef struct AudioContext {
    SDL_AudioDeviceID dev;
    AudioDecoder* audio_decoder;
    int decoder_frequency;
} AudioContext;

// Audio Rendering
typedef struct RenderContext {
    // Whether or not the audio is encoded
    bool encoded;
    // Raw audio packets
    AudioPacket audio_data[MAX_NUM_AUDIO_INDICES];
} RenderContext;

AudioContext volatile audio_context;

clock nack_timer;

int last_nacked_id = -1;
int most_recent_audio_id = -1;
int last_played_id = -1;

// Audio flush happens when the audio buffer gets too large, and we need to clear it out
bool audio_flush_triggered = false;

// Audio refresh happens when the user plugs in a new audio device,
// or unplugs their current audio device
bool audio_refresh = false;

#define MAX_FREQ 128000

clock test_timer;
double test_time;

static RenderContext volatile render_context;
static bool volatile rendering = false;

/*
============================
Private Function Implementations
============================
*/

void destroy_audio_device() {
    /*
        Destroys the audio device
    */
    if (audio_context.dev) {
        SDL_CloseAudioDevice(audio_context.dev);
        audio_context.dev = 0;
    }
    if (audio_context.audio_decoder) {
        destroy_audio_decoder(audio_context.audio_decoder);
        audio_context.audio_decoder = NULL;
    }
}

void reinit_audio_device() {
    LOG_INFO("Reinitializing audio device");
    destroy_audio_device();

    // cast socket and SDL variables back to their data type for usage
    SDL_AudioSpec wanted_spec = {0}, audio_spec = {0};
    audio_context.decoder_frequency = audio_frequency;
    audio_context.audio_decoder = create_audio_decoder(audio_context.decoder_frequency);

    SDL_zero(wanted_spec);
    SDL_zero(audio_spec);
    wanted_spec.channels = 2;
    wanted_spec.freq = audio_context.decoder_frequency;
    LOG_INFO("Audio Freqency: %d", wanted_spec.freq);
    wanted_spec.format = AUDIO_F32SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;

    audio_context.dev =
        SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (audio_context.dev == 0) {
        LOG_ERROR("Failed to open audio: %s", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(audio_context.dev, 0);
    }
}

/*
============================
Public Function Implementations
============================
*/

void init_audio() {
    /*
        Initialize the audio device
    */
    LOG_INFO("Initializing audio system");
    start_timer(&nack_timer);
    for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
        receiving_audio[i].id = -1;
        receiving_audio[i].nacked_amount = 0;
        receiving_audio[i].nacked_for = -1;
    }
    // Set audio to be reinit'ed
    audio_refresh = true;
}

void destroy_audio() {
    LOG_INFO("Destroying audio system");
    destroy_audio_device();
}

void render_audio() {
    if (rendering) {
        if (audio_frequency > MAX_FREQ) {
            LOG_ERROR("Frequency received was too large: %d, silencing audio now.",
                      audio_frequency);
            audio_frequency = -1;
        }

        // If no audio frequency has been received yet, then don't render the audio
        if (audio_frequency < 0) {
            rendering = false;
            return;
        }

        if (audio_context.decoder_frequency != audio_frequency) {
            LOG_INFO("Updating audio frequency to %d!", audio_frequency);
            audio_refresh = true;
        }

        if (audio_refresh) {
            // This gap between if audio_refresh == true and audio_refresh=false creates a minor
            // race condition with sdl_event_handler.c trying to refresh the audio when the audio
            // device has changed.
            audio_refresh = false;
            reinit_audio_device();
        }

        if (render_context.encoded) {
            AVPacket encoded_packet;
            av_init_packet(&encoded_packet);
            encoded_packet.data = (uint8_t*)av_malloc(MAX_NUM_AUDIO_INDICES * MAX_PAYLOAD_SIZE);
            encoded_packet.size = 0;

            for (int i = 0; i < MAX_NUM_AUDIO_INDICES; i++) {
                AudioPacket* packet = (AudioPacket*)&render_context.audio_data[i];
                memcpy(encoded_packet.data + encoded_packet.size, packet->data, packet->size);
                encoded_packet.size += packet->size;
            }

            // Decode encoded audio
            int res = audio_decoder_decode_packet(audio_context.audio_decoder, &encoded_packet);
            av_free(encoded_packet.data);
            av_packet_unref(&encoded_packet);

            if (res == 0) {
                uint8_t decoded_data[MAX_AUDIO_FRAME_SIZE];

                // Get decoded data
                audio_decoder_packet_readout(audio_context.audio_decoder, decoded_data);

                // Play decoded audio
                res =
                    SDL_QueueAudio(audio_context.dev, &decoded_data,
                                   audio_decoder_get_frame_data_size(audio_context.audio_decoder));

                if (res < 0) {
                    LOG_ERROR("Could not play audio!");
                }
            }
        } else {
            // Play raw audio
            for (int i = 0; i < MAX_NUM_AUDIO_INDICES; i++) {
                AudioPacket* packet = (AudioPacket*)&render_context.audio_data[i];
                if (packet->size > 0) {
#if LOG_AUDIO
                    LOG_DEBUG("Playing Audio ID %d (Size: %d) (Queued: %d)\n", packet->id,
                              packet->size, SDL_GetQueuedAudioSize(audio_context.dev));
#endif
                    if (SDL_QueueAudio(audio_context.dev, packet->data, packet->size) < 0) {
                        LOG_ERROR("Could not play audio!\n");
                    }
                }
            }
        }
        // No longer rendering audio
        rendering = false;
    }
}

void update_audio() {
    /*
        This function will create or reinit the audio device if needed,
        and it will play any queued audio packets.
    */

    // If we're currently rendering an audio packet, don't update audio
    if (rendering) {
        // Additionally, if rendering == true, the audio_context struct is being used,
        // so a race condition will occur if call SDL_GetQueuedAudioSize at the same time
        return;
    }

    int audio_device_queue = 0;
    if (audio_context.dev) {
        // If we have a device, get the queue size
        audio_device_queue = (int)SDL_GetQueuedAudioSize(audio_context.dev);
    }  // Otherwise, the queue size is 0

#if LOG_AUDIO
    LOG_DEBUG("Queue: %d", audio_device_queue);
#endif
    bool still_more_audio_packets = true;

    // Catch up to most recent ID if nothing has played yet
    if (last_played_id == -1 && has_rendered_yet && most_recent_audio_id > 0) {
        last_played_id = most_recent_audio_id - 1;
        while (last_played_id % MAX_NUM_AUDIO_INDICES != MAX_NUM_AUDIO_INDICES - 1) {
            last_played_id++;
        }

        // Clean out the old packets
        for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
            if (receiving_audio[i].id <= last_played_id) {
                receiving_audio[i].id = -1;
                receiving_audio[i].nacked_amount = 0;
            }
        }
    }

    // Buffering audio controls whether or not we're trying to accumulate an audio buffer
    static bool buffering_audio = false;

    int bytes_until_no_more_audio =
        (most_recent_audio_id - last_played_id) * MAX_PAYLOAD_SIZE + audio_device_queue;

    // If the audio queue is under AUDIO_QUEUE_LOWER_LIMIT
    if (!buffering_audio && bytes_until_no_more_audio < AUDIO_QUEUE_LOWER_LIMIT) {
        LOG_INFO("Audio Queue too low: %d. Needs to catch up!", bytes_until_no_more_audio);
        buffering_audio = true;
    }

    if (buffering_audio) {
        if (bytes_until_no_more_audio < TARGET_AUDIO_QUEUE_LIMIT) {
            return;
        } else {
            LOG_INFO("Done catching up! Audio Queue: %d", bytes_until_no_more_audio);
            buffering_audio = false;
        }
    }

    if (last_played_id == -1) {
        return;
    }

    int next_to_play_id;

    while (still_more_audio_packets) {
        next_to_play_id = last_played_id + 1;

        if (next_to_play_id % MAX_NUM_AUDIO_INDICES != 0) {
            LOG_WARNING("NEXT TO PLAY ISN'T AT START OF AUDIO FRAME!");
            return;
        }

        bool valid = true;
        for (int i = next_to_play_id; i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
            if (receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id != i) {
                valid = false;
            }
        }

        still_more_audio_packets = false;
        if (valid) {
            // dont keep checking for more audio packets
            // still_more_audio_packets = true;

            // If an audio flush is triggered,
            // We skip audio until the buffer runs down to TARGET_AUDIO_QUEUE_LIMIT
            int real_limit =
                audio_flush_triggered ? TARGET_AUDIO_QUEUE_LIMIT : AUDIO_QUEUE_UPPER_LIMIT;

            if (audio_device_queue > real_limit) {
                LOG_WARNING("Audio queue full, skipping ID %d (Queued: %d)",
                            next_to_play_id / MAX_NUM_AUDIO_INDICES, audio_device_queue);
                // Clear out audio instead of playing it
                for (int i = next_to_play_id; i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
                    AudioPacket* packet = &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }
                // Trigger an audio flush when the audio queue surpasses AUDIO_QUEUE_UPPER_LIMIT
                if (!audio_flush_triggered) {
                    audio_flush_triggered = true;
                }
            } else {
                // When the audio queue is no longer full, we stop flushing the audio queue
                audio_flush_triggered = false;

                // Store the audio render context information
                render_context.encoded = USING_AUDIO_ENCODE_DECODE;
                for (int i = 0; i < MAX_NUM_AUDIO_INDICES; i++) {
                    int buffer_index = next_to_play_id + i;
                    // Save buffer in render context
                    // fprintf(stderr, "%p %p %ld\n", &render_context.audio_data[i],
                    // &receiving_audio[buffer_index % RECV_AUDIO_BUFFER_SIZE],
                    // sizeof(AudioPacket));
                    memcpy((AudioPacket*)&render_context.audio_data[i],
                           &receiving_audio[buffer_index % RECV_AUDIO_BUFFER_SIZE],
                           sizeof(AudioPacket));
                    // Reset packet in receiving audio buffer
                    AudioPacket* packet = &receiving_audio[buffer_index % RECV_AUDIO_BUFFER_SIZE];
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }

                // Render audio on audio thread
                rendering = true;
            }

            last_played_id += MAX_NUM_AUDIO_INDICES;
        }
    }

    //         }

    //         last_played_id += MAX_NUM_AUDIO_INDICES;
    //     }
    // }

    // Find pending audio packets and NACK them
#define MAX_NACKED 1
    unsigned char fmsg_buffer[MAX_NACKED * sizeof(FractalClientMessage)];
    FractalClientMessage* fmsg[MAX_NACKED];
    int num_nacked = 0;
    if (last_played_id > -1 && get_timer(nack_timer) > 6.0 / MS_IN_SECOND) {
        last_nacked_id = max(last_played_id, last_nacked_id);
        for (int i = last_nacked_id + 1; i < most_recent_audio_id - 4 && num_nacked < MAX_NACKED;
             i++) {
            int i_buffer_index = i % RECV_AUDIO_BUFFER_SIZE;
            AudioPacket* i_packet = &receiving_audio[i_buffer_index];
            if (i_packet->id == -1 && i_packet->nacked_amount < 2) {
                i_packet->nacked_amount++;
                // Set fmsg pointer to location in buffer
                fmsg[num_nacked] =
                    (FractalClientMessage*)(fmsg_buffer +
                                            num_nacked * sizeof(FractalClientMessage));
                // Populate FractalClientMessage
                fmsg[num_nacked]->type = MESSAGE_AUDIO_NACK;
                fmsg[num_nacked]->nack_data.id = i / MAX_NUM_AUDIO_INDICES;
                fmsg[num_nacked]->nack_data.index = i % MAX_NUM_AUDIO_INDICES;
                i_packet->nacked_for = i;
                num_nacked++;

                start_timer(&nack_timer);
            }
            last_nacked_id = i;
        }
    }

    // Send nacks out
    for (int i = 0; i < num_nacked; i++) {
        LOG_INFO("Missing Audio Packet ID %d, Index %d. NACKing...", fmsg[i]->nack_data.id,
                 fmsg[i]->nack_data.index);
        send_fmsg(fmsg[i]);
    }
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int32_t receive_audio(FractalPacket* packet) {
    /*
        This function will store an audio packet in its internal buffer, to be played on later
        calls to update_audio. A buffer is needed so that the audio comes out smoothly. We delay the
        audio by about 30ms to ensure that the buffer is never empty while we call update_audio. If
        the buffer is empty, the speakers will make a "pop" noise.

        Arguments:
            packet (FractalPacket*): FractalPacket wrapped audio data.

        Return:
            ret (int): 0 on success, -1 on failure
    */
    if (packet->index >= MAX_NUM_AUDIO_INDICES) {
        LOG_WARNING("Packet Index too large!");
        return -1;
    }
    if (audio_frequency == MAX_FREQ) {
        return 0;
    }

    int audio_id = packet->id * MAX_NUM_AUDIO_INDICES + packet->index;
    AudioPacket* audio_pkt = &receiving_audio[audio_id % RECV_AUDIO_BUFFER_SIZE];

    if (audio_id == audio_pkt->id) {
        //         LOG_WARNING("Already received audio packet: %d", audio_id);
    } else if (audio_id < audio_pkt->id || audio_id <= last_played_id) {
        // LOG_INFO("Old audio packet received: %d, last played id is %d",
        // audio_id, last_played_id);
    }
    // audio_id > audio_pkt->id && audio_id > last_played_id
    else {
        // If a packet already exists there, we're forced to skip it
        if (audio_pkt->id != -1) {
            int old_last_played_id = last_played_id;

            if (last_played_id < audio_pkt->id && last_played_id > 0) {
                // We'll make it like we already played this packet
                last_played_id = audio_pkt->id;
                audio_pkt->id = -1;
                audio_pkt->nacked_amount = 0;

                // And we'll skip the whole frame
                while (last_played_id % MAX_NUM_AUDIO_INDICES != MAX_NUM_AUDIO_INDICES - 1) {
                    // "Play" that packet
                    last_played_id++;
                    receiving_audio[last_played_id % RECV_AUDIO_BUFFER_SIZE].id = -1;
                    receiving_audio[last_played_id % RECV_AUDIO_BUFFER_SIZE].nacked_amount = 0;
                }
            }

            if (last_played_id > 0) {
                LOG_INFO(
                    "Audio packet being overwritten before being played! ID %d "
                    "replaced with ID %d, when the Last Played ID was %d. Last "
                    "Played ID is Now %d",
                    audio_pkt->id, audio_id, old_last_played_id, last_played_id);
            }
        }

        if (packet->is_a_nack) {
            if (audio_pkt->nacked_for == audio_id) {
                LOG_INFO("NACK for Audio ID %d, Index %d Received!", packet->id, packet->index);
            } else if (audio_pkt->nacked_for == -1) {
                LOG_INFO(
                    "NACK for Audio ID %d, Index %d Received! But not "
                    "needed.",
                    packet->id, packet->index);
            } else {
                LOG_ERROR("NACK for Audio ID %d, Index %d Received, but of unexpected index?",
                          packet->id, packet->index);
            }
        }

        if (audio_pkt->nacked_for == audio_id) {
            LOG_INFO(
                "Packet for Audio ID %d, Index %d Received! But it was already "
                "NACK'ed!",
                packet->id, packet->index);
        }
        audio_pkt->nacked_for = -1;

#if LOG_AUDIO
        LOG_DEBUG("Receiving Audio Packet %d (%d), trying to render %d (Queue: %d)\n", audio_id,
                  packet->payload_size, last_played_id + 1, audio_device_queue);
#endif
        audio_pkt->id = audio_id;
        most_recent_audio_id = max(audio_pkt->id, most_recent_audio_id);
        audio_pkt->size = packet->payload_size;
        memcpy(audio_pkt->data, packet->data, packet->payload_size);

        if (packet->index + 1 == packet->num_indices) {
            // LOG_INFO("Audio Packet %d Received! Last Played: %d",
            // packet->id, last_played_id / MAX_NUM_AUDIO_INDICES);
            for (int i = audio_id + 1; i % MAX_NUM_AUDIO_INDICES != 0; i++) {
                // LOG_INFO("Receiving %d", i);
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = i;
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].size = 0;
            }
        }
    }

    return 0;
}
