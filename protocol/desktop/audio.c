#include "audio.h"

// Hold information about audio data as the packets come in
typedef struct audio_packet {
    int id;
    int size;
    int nacked_for;
    char data[MAX_PACKET_SIZE];
} audio_packet;

#define RECV_AUDIO_BUFFER_SIZE 50
#define MAX_NUM_AUDIO_INDICES 5
audio_packet receiving_audio[RECV_AUDIO_BUFFER_SIZE];

#define SDL_AUDIO_BUFFER_SIZE 1024

struct AudioData {
    SDL_AudioDeviceID dev;
    audio_decoder_t* audio_decoder;
    int last_played_id;
} volatile AudioData;

void initAudio() {
    // cast socket and SDL variables back to their data type for usage
    SDL_AudioSpec wantedSpec = { 0 }, audioSpec = { 0 };

    AudioData.audio_decoder = create_audio_decoder();

    SDL_zero(wantedSpec);
    SDL_zero(audioSpec);
    wantedSpec.channels  = AudioData.audio_decoder->context->channels;
    wantedSpec.freq      = AudioData.audio_decoder->context->sample_rate;
    wantedSpec.format    = AUDIO_F32SYS;
    wantedSpec.silence   = 0;
    wantedSpec.samples   = SDL_AUDIO_BUFFER_SIZE;

    AudioData.dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &audioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioData.dev == 0) {
        mprintf("Failed to open audio\n");
        exit(1);
    }

    SDL_PauseAudioDevice(AudioData.dev, 0);

    AudioData.last_played_id = -1;

    for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
        receiving_audio[i].id = -1;
        receiving_audio[i].nacked_for = -1;
    }
}

void destroyAudio() {
    SDL_CloseAudioDevice(AudioData.dev);
    destroy_audio_decoder(AudioData.audio_decoder);
}

int last_nacked_id = -1;
int most_recent_audio_id = -1;

void updateAudio() {
    bool still_more_audio_packets = true;
    int next_to_play_id;
    int next_to_play_buffer_index;
    while (still_more_audio_packets) {
        // Catch up to most recent ID if nothing has played yet
        if (AudioData.last_played_id == -1 && most_recent_audio_id > 0) {
            AudioData.last_played_id = most_recent_audio_id - 1;
            // Clean out the old packets
            for (int i = 0; i <= AudioData.last_played_id; i++) {
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = -1;
            }
        }

        next_to_play_id = AudioData.last_played_id + 1;
        next_to_play_buffer_index = next_to_play_id % RECV_AUDIO_BUFFER_SIZE;
        audio_packet* packet = &receiving_audio[next_to_play_buffer_index];

        still_more_audio_packets = false;
        if (packet->id == next_to_play_id) {
            still_more_audio_packets = true;
            //mprintf("Playing ID %d\n", packet->id);
            AudioData.last_played_id = next_to_play_id;
            if (packet->size > 0) {
                SDL_QueueAudio(AudioData.dev, packet->data, packet->size);
            }
            packet->id = -1;
        }
    }

    next_to_play_id = AudioData.last_played_id + 1;
    next_to_play_buffer_index = next_to_play_id % RECV_AUDIO_BUFFER_SIZE;

    if (most_recent_audio_id > next_to_play_id) {
        // Find all pending audio packets and NACK them
        for (int i = max(next_to_play_id, last_nacked_id + 1); i < most_recent_audio_id; i++) {
            int i_buffer_index = i % RECV_AUDIO_BUFFER_SIZE;
            audio_packet* i_packet = &receiving_audio[i_buffer_index];
            if (i_packet->id == -1) {
                FractalClientMessage fmsg;
                fmsg.type = MESSAGE_AUDIO_NACK;
                fmsg.nack_data.id = i / MAX_NUM_AUDIO_INDICES;
                fmsg.nack_data.index = i % MAX_NUM_AUDIO_INDICES;
                mprintf("Missing ID %d, Index %d. NACKing...\n", fmsg.nack_data.id, fmsg.nack_data.index);
                i_packet->nacked_for = i;
                SendPacket(&fmsg, sizeof(fmsg));
            }
        }
        last_nacked_id = most_recent_audio_id - 1;
    }
}

int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size) {
    int audio_id = packet->id * MAX_NUM_AUDIO_INDICES + packet->index;
    audio_packet* audio_pkt = &receiving_audio[audio_id % RECV_AUDIO_BUFFER_SIZE];

    if (audio_id > audio_pkt->id && audio_id > AudioData.last_played_id) {
        if (audio_pkt->id != -1) {
            mprintf("Audio packet being overwritten before being played! ID %d replaced with ID %d, when the Last Played ID was %d\n", audio_pkt->id, audio_id, AudioData.last_played_id);
            AudioData.last_played_id = audio_id - 1;
            for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
                if (receiving_audio[i].id <= AudioData.last_played_id) {
                    receiving_audio[i].id = -1;
                }
            }
        }

        if (audio_pkt->nacked_for == audio_id) {
            mprintf("NACK for ID %d, Index %d Received!\n", packet->id, packet->index);
        }
        audio_pkt->nacked_for = -1;
        audio_pkt->id = audio_id;
        most_recent_audio_id = max(audio_pkt->id, most_recent_audio_id);
        audio_pkt->size = packet->payload_size;
        memcpy(audio_pkt->data, packet->data, packet->payload_size);

        if (packet->is_ending) {
            //mprintf("Audio Packet %d Received! Last Played: %d\n", packet->id, AudioData.last_played_id);
            for (int i = audio_id + 1; i % MAX_NUM_AUDIO_INDICES != 0; i++) {
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = i;
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].size = 0;
            }
        }
    }

    return 0;
}