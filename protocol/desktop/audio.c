#include "audio.h"

extern int audio_frequency;
extern bool has_rendered_yet;

// Hold information about audio data as the packets come in
typedef struct audio_packet {
    int id;
    int size;
    int nacked_for;
    int nacked_amount;
    char data[MAX_PAYLOAD_SIZE];
} audio_packet;

#define LOG_AUDIO false

#define AUDIO_QUEUE_LOWER_LIMIT 18000
#define AUDIO_QUEUE_UPPER_LIMIT 59000
#define TARGET_AUDIO_QUEUE_LIMIT 28000

#define MAX_NUM_AUDIO_FRAMES 25
#define MAX_NUM_AUDIO_INDICES 3
#define RECV_AUDIO_BUFFER_SIZE (MAX_NUM_AUDIO_FRAMES * MAX_NUM_AUDIO_INDICES)
audio_packet receiving_audio[RECV_AUDIO_BUFFER_SIZE];

#define SDL_AUDIO_BUFFER_SIZE 1024

struct AudioData {
    SDL_AudioDeviceID dev;
    audio_decoder_t* audio_decoder;
} volatile AudioData;

clock nack_timer;

int last_nacked_id = -1;
int most_recent_audio_id = -1;
int last_played_id = -1;

bool triggered = false;

int decoder_frequency = 48000;

clock test_timer;
double test_time;

void initAudio() {
    StartTimer(&nack_timer);

    // cast socket and SDL variables back to their data type for usage
    SDL_AudioSpec wantedSpec = {0}, audioSpec = {0};

    AudioData.audio_decoder = create_audio_decoder(decoder_frequency);

    SDL_zero(wantedSpec);
    SDL_zero(audioSpec);
    wantedSpec.channels =
        2;  // 2;//(Uint8)AudioData.audio_decoder->pCodecCtx->channels;
    wantedSpec.freq = decoder_frequency;
    LOG_INFO("Freq: %d", wantedSpec.freq);
    wantedSpec.format = AUDIO_F32SYS;
    wantedSpec.silence = 0;
    wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;

    AudioData.dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec,
                                        SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioData.dev == 0) {
        LOG_ERROR("Failed to open audio");
        exit(1);
    }

    SDL_PauseAudioDevice(AudioData.dev, 0);

    for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
        receiving_audio[i].id = -1;
        receiving_audio[i].nacked_amount = 0;
        receiving_audio[i].nacked_for = -1;
    }
}

void destroyAudio() {
    SDL_CloseAudioDevice(AudioData.dev);
    if (AudioData.audio_decoder) {
        destroy_audio_decoder(AudioData.audio_decoder);
    }
}

void updateAudio() {
#if LOG_AUDIO
    // mprintf("Queue: %d\n", SDL_GetQueuedAudioSize(AudioData.dev));
#endif
    if (audio_frequency > 0 && decoder_frequency != audio_frequency) {
        LOG_INFO("Updating audio frequency to %d!", audio_frequency);
        decoder_frequency = audio_frequency;
        destroyAudio();
        initAudio();
    }

    bool still_more_audio_packets = true;

    // Catch up to most recent ID if nothing has played yet
    if (last_played_id == -1 && has_rendered_yet && most_recent_audio_id > 0) {
        last_played_id = most_recent_audio_id - 1;
        while (last_played_id % MAX_NUM_AUDIO_INDICES !=
               MAX_NUM_AUDIO_INDICES - 1) {
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

    // Wait to delay
    static bool gapping = false;
    int bytes_until_can_play =
        (most_recent_audio_id - last_played_id) * MAX_PAYLOAD_SIZE +
        SDL_GetQueuedAudioSize(AudioData.dev);
    if (!gapping && bytes_until_can_play < AUDIO_QUEUE_LOWER_LIMIT) {
        LOG_INFO("Audio Queue too low: %d. Needs to catch up!",
                 bytes_until_can_play);
        gapping = true;
    }

    if (gapping) {
        if (bytes_until_can_play < TARGET_AUDIO_QUEUE_LIMIT) {
            return;
        } else {
            LOG_INFO("Done catching up! Audio Queue: %d", bytes_until_can_play);
            gapping = false;
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
        for (int i = next_to_play_id;
             i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
            if (receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id != i) {
                valid = false;
            }
        }

        still_more_audio_packets = false;
        if (valid) {
            still_more_audio_packets = true;

            int real_limit =
                triggered ? TARGET_AUDIO_QUEUE_LIMIT : AUDIO_QUEUE_UPPER_LIMIT;

            if (SDL_GetQueuedAudioSize(AudioData.dev) >
                (unsigned int)real_limit) {
                //                mprintf("*******************");
                LOG_WARNING("Audio queue full, skipping ID %d (Queued: %d)",
                            next_to_play_id / MAX_NUM_AUDIO_INDICES,
                            SDL_GetQueuedAudioSize(AudioData.dev));
                for (int i = next_to_play_id;
                     i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
                    audio_packet* packet =
                        &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }
                triggered = true;
            } else {
                triggered = false;
#if USING_AUDIO_ENCODE_DECODE
                AVPacket encoded_packet;
                int res;
                av_init_packet(&encoded_packet);
                encoded_packet.data = (uint8_t*)av_malloc(
                    MAX_NUM_AUDIO_INDICES * MAX_PAYLOAD_SIZE);
                encoded_packet.size = 0;

                for (int i = next_to_play_id;
                     i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
                    audio_packet* packet =
                        &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
                    memcpy(encoded_packet.data + encoded_packet.size,
                           packet->data, packet->size);
                    encoded_packet.size += packet->size;
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }
                res = audio_decoder_decode_packet(AudioData.audio_decoder,
                                                  &encoded_packet);
                av_free(encoded_packet.data);
                av_packet_unref(&encoded_packet);

                if (res == 0) {
                    uint8_t decoded_data[MAX_AUDIO_FRAME_SIZE];

                    audio_decoder_packet_readout(AudioData.audio_decoder,
                                                 decoded_data);

                    res = SDL_QueueAudio(AudioData.dev, &decoded_data,
                                         audio_decoder_get_frame_data_size(
                                             AudioData.audio_decoder));

                    if (res < 0) {
                        LOG_WARNING("Could not play audio!");
                    }
                }
#else
                for (int i = next_to_play_id;
                     i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
                    audio_packet* packet =
                        &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
                    if (packet->size > 0) {
#if LOG_AUDIO
                        mprintf("Playing Audio ID %d (Size: %d) (Queued: %d)\n",
                                packet->id, packet->size,
                                SDL_GetQueuedAudioSize(AudioData.dev));
#endif
                        if (SDL_QueueAudio(AudioData.dev, packet->data,
                                           packet->size) < 0) {
                            mprintf("Could not play audio!\n");
                        }
                    }
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }
#endif
            }

            last_played_id += MAX_NUM_AUDIO_INDICES;
        }
    }

    //         }

    //         last_played_id += MAX_NUM_AUDIO_INDICES;
    //     }
    // }

    // Find all pending audio packets and NACK them
    if (last_played_id > -1 && GetTimer(nack_timer) > 6.0 / 1000.0) {
        int num_nacked = 0;
        last_nacked_id = max(last_played_id, last_nacked_id);
        for (int i = last_nacked_id + 1;
             i < most_recent_audio_id - 4 && num_nacked < 1; i++) {
            int i_buffer_index = i % RECV_AUDIO_BUFFER_SIZE;
            audio_packet* i_packet = &receiving_audio[i_buffer_index];
            if (i_packet->id == -1 && i_packet->nacked_amount < 2) {
                i_packet->nacked_amount++;
                FractalClientMessage fmsg;
                fmsg.type = MESSAGE_AUDIO_NACK;
                fmsg.nack_data.id = i / MAX_NUM_AUDIO_INDICES;
                fmsg.nack_data.index = i % MAX_NUM_AUDIO_INDICES;
                LOG_INFO("Missing Audio Packet ID %d, Index %d. NACKing...",
                         fmsg.nack_data.id, fmsg.nack_data.index);
                i_packet->nacked_for = i;
                SendFmsg(&fmsg);
                num_nacked++;

                StartTimer(&nack_timer);
            }
            last_nacked_id = i;
        }
    }
}

int32_t ReceiveAudio(FractalPacket* packet) {
    if (packet->index >= MAX_NUM_AUDIO_INDICES) {
        LOG_WARNING("Packet Index too large!");
        return -1;
    }

    int audio_id = packet->id * MAX_NUM_AUDIO_INDICES + packet->index;
    audio_packet* audio_pkt =
        &receiving_audio[audio_id % RECV_AUDIO_BUFFER_SIZE];

    if (audio_id == audio_pkt->id) {
        //         mprintf("Already received audio packet: %d\n", audio_id);
    } else if (audio_id < audio_pkt->id || audio_id <= last_played_id) {
        // mprintf("Old audio packet received: %d, last played id is %d\n",
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
                while (last_played_id % MAX_NUM_AUDIO_INDICES !=
                       MAX_NUM_AUDIO_INDICES - 1) {
                    // "Play" that packet
                    last_played_id++;
                    receiving_audio[last_played_id % RECV_AUDIO_BUFFER_SIZE]
                        .id = -1;
                    receiving_audio[last_played_id % RECV_AUDIO_BUFFER_SIZE]
                        .nacked_amount = 0;
                }
            }

            if (last_played_id > 0) {
                (void)old_last_played_id;  // suppress unused variable warning.
                LOG_INFO(
                    "Audio packet being overwritten before being played! ID %d "
                    "replaced with ID %d, when the Last Played ID was %d. Last "
                    "Played ID is Now %d",
                    audio_pkt->id, audio_id, old_last_played_id,
                    last_played_id);
            }
        }

        if (packet->is_a_nack) {
            if (audio_pkt->nacked_for == audio_id) {
                LOG_INFO("NACK for Audio ID %d, Index %d Received!", packet->id,
                         packet->index);
            } else if (audio_pkt->nacked_for == -1) {
                LOG_INFO(
                    "NACK for Audio ID %d, Index %d Received! But not "
                    "needed.",
                    packet->id, packet->index);
            } else {
                LOG_WARNING("NACK for Audio ID %d, Index %d Received Wrongly!",
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
        mprintf(
            "Receiving Audio Packet %d (%d), trying to render %d (Queue: %d)\n",
            audio_id, packet->payload_size, last_played_id + 1,
            SDL_GetQueuedAudioSize(AudioData.dev));
#endif
        audio_pkt->id = audio_id;
        most_recent_audio_id = max(audio_pkt->id, most_recent_audio_id);
        audio_pkt->size = packet->payload_size;
        memcpy(audio_pkt->data, packet->data, packet->payload_size);

        if (packet->index + 1 == packet->num_indices) {
            // mprintf("Audio Packet %d Received! Last Played: %d\n",
            // packet->id, last_played_id / MAX_NUM_AUDIO_INDICES);
            for (int i = audio_id + 1; i % MAX_NUM_AUDIO_INDICES != 0; i++) {
                // mprintf("Receiving %d\n", i);
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = i;
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].size = 0;
            }
        }
    }

    return 0;
}
