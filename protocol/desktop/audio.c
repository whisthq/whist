#include "audio.h"

// Hold information about audio data as the packets come in
typedef struct audio_packet {
    int id;
    int size;
    int nacked_for;
    int nacked_amount;
    char data[MAX_PAYLOAD_SIZE];
} audio_packet;

#define AUDIO_QUEUE_LIMIT 50000
#define TRIGGERED_AUDIO_QUEUE_LIMIT 30000
#define MAX_NUM_AUDIO_FRAMES 10
#define MAX_NUM_AUDIO_INDICES 10
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

void initAudio() {
    StartTimer(&nack_timer);

    // cast socket and SDL variables back to their data type for usage
    SDL_AudioSpec wantedSpec = { 0 }, audioSpec = { 0 };

    AudioData.audio_decoder = create_audio_decoder();

    SDL_zero(wantedSpec);
    SDL_zero(audioSpec);
    wantedSpec.channels  = AudioData.audio_decoder->context->channels;
    wantedSpec.freq      = 44100;//AudioData.audio_decoder->context->sample_rate * 1.05;
    mprintf("Freq: %d\n", wantedSpec.freq);
    wantedSpec.format    = AUDIO_F32SYS;
    wantedSpec.silence   = 0;
    wantedSpec.samples   = SDL_AUDIO_BUFFER_SIZE;

    AudioData.dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioData.dev == 0) {
        mprintf("Failed to open audio\n");
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
    destroy_audio_decoder(AudioData.audio_decoder);
}

void updateAudio() {
    bool still_more_audio_packets = true;

    // Catch up to most recent ID if nothing has played yet
    if (last_played_id == -1 && most_recent_audio_id > 0) {
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

    if (last_played_id == -1) {
        return;
    }

    int next_to_play_id;

    while (still_more_audio_packets) {
        next_to_play_id = last_played_id + 1;

        if (next_to_play_id % MAX_NUM_AUDIO_INDICES != 0) {
            mprintf("NEXT TO PLAY ISN'T AT START OF AUDIO FRAME!\n");
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
            still_more_audio_packets = true;

            int real_limit = triggered ? TRIGGERED_AUDIO_QUEUE_LIMIT : AUDIO_QUEUE_LIMIT;

            if (SDL_GetQueuedAudioSize(AudioData.dev) > real_limit) {
                mprintf("Audio queue full, catching up by one frame\n");
                for (int i = next_to_play_id; i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
                    audio_packet* packet = &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }
                triggered = true;
            }
            else {
                triggered = false;
                for (int i = next_to_play_id; i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
                    audio_packet* packet = &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
                    if (packet->size > 0) {
                            mprintf("Playing %d (%d) (%d)\n", packet->id, packet->size, SDL_GetQueuedAudioSize(AudioData.dev));
                            if (SDL_QueueAudio(AudioData.dev, packet->data, packet->size) < 0) {
                                mprintf("Could not play audio!\n");
                            }
                    }
                    packet->id = -1;
                    packet->nacked_amount = 0;
                }
            }

            last_played_id = last_played_id += MAX_NUM_AUDIO_INDICES;
        }
    }

    if (last_played_id > -1) {
        if (GetTimer(nack_timer) > 2.0 / 1000.0) {
            StartTimer(&nack_timer);
            last_nacked_id = last_played_id;
        }

        next_to_play_id = last_played_id + 1;

        // Find all pending audio packets and NACK them
        for (int i = max(next_to_play_id, last_nacked_id + 1); i < most_recent_audio_id; i++) {
            int i_buffer_index = i % RECV_AUDIO_BUFFER_SIZE;
            audio_packet* i_packet = &receiving_audio[i_buffer_index];
            if (i_packet->id == -1 && i_packet->nacked_amount < 2) {
                i_packet->nacked_amount++;
                FractalClientMessage fmsg;
                fmsg.type = MESSAGE_AUDIO_NACK;
                fmsg.nack_data.id = i / MAX_NUM_AUDIO_INDICES;
                fmsg.nack_data.index = i % MAX_NUM_AUDIO_INDICES;
                //mprintf("Missing Audio Packet ID %d, Index %d. NACKing...\n", fmsg.nack_data.id, fmsg.nack_data.index);
                i_packet->nacked_for = i;
                SendPacket(&fmsg, sizeof(fmsg));
            }
            last_nacked_id = i;
        }
    }
}

int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size) {
    if (packet->index >= MAX_NUM_AUDIO_INDICES) {
        mprintf("Packet Index too large!\n");
        return -1;
    }

    int audio_id = packet->id * MAX_NUM_AUDIO_INDICES + packet->index;
    audio_packet* audio_pkt = &receiving_audio[audio_id % RECV_AUDIO_BUFFER_SIZE];

    if (audio_id == audio_pkt->id) {
        //mprintf("Already received audio packet: %d\n", audio_id);
    }
    else if (audio_id < audio_pkt->id || audio_id <= last_played_id) {
        //mprintf("Old audio packet received: %d\n", audio_id);
        //mprintf("Audio Packet ID %d\n last played %d\n", audio_pkt->id, last_played_id);
    }
    // audio_id > audio_pkt->id && audio_id > last_played_id
    else {
        // If a packet already exists there, we're forced to skip it
        if (audio_pkt->id != -1) {
            int old_last_played_id = last_played_id;

            if (last_played_id < audio_pkt->id) {
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

            mprintf("Audio packet being overwritten before being played! ID %d replaced with ID %d, when the Last Played ID was %d. Last Played ID is Now %d\n", audio_pkt->id, audio_id, old_last_played_id, last_played_id);
        }

        if (audio_pkt->nacked_for == audio_id) {
            //mprintf("NACK for Audio ID %d, Index %d Received!\n", packet->id, packet->index);
        }
        audio_pkt->nacked_for = -1;

        //mprintf("Receiving %d\n", audio_id);
        audio_pkt->id = audio_id;
        most_recent_audio_id = max(audio_pkt->id, most_recent_audio_id);
        audio_pkt->size = packet->payload_size;
        memcpy(audio_pkt->data, packet->data, packet->payload_size);

        if (packet->index + 1 == packet->num_indices) {
            //mprintf("Audio Packet %d Received! Last Played: %d\n", packet->id, last_played_id);
            for (int i = audio_id + 1; i % MAX_NUM_AUDIO_INDICES != 0; i++) {
                //mprintf("Receiving %d\n", i);
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = i;
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].size = 0;
            }
        }
    }

    return 0;
}