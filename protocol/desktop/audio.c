#include "audio.h"

// Hold information about audio data as the packets come in
typedef struct audio_packet {
    int id;
    int size;
    char data[MAX_PACKET_SIZE];
} audio_packet;

#define RECV_AUDIO_BUFFER_SIZE 100
#define MAX_NUM_AUDIO_INDICES 10
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
    }
}

void destroyAudio() {
    SDL_CloseAudioDevice(AudioData.dev);
    destroy_audio_decoder(AudioData.audio_decoder);
}

void updateQueuedAudio() {
    while (true) {
        int next_to_play_id = AudioData.last_played_id + 1;
        int next_to_play_buffer_index = next_to_play_id % RECV_AUDIO_BUFFER_SIZE;
        audio_packet* packet = &receiving_audio[next_to_play_buffer_index];
        if (packet->id == next_to_play_id) {
            //mprintf("Playing ID %d\n", packet->id);
            AudioData.last_played_id = next_to_play_id;
            if (packet->size > 0) {
                SDL_QueueAudio(AudioData.dev, packet->data, packet->size);
            }
            packet->id = -1;
        }
        else {
            break;
        }
    }
}

int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size) {
    int audio_id = packet->id * MAX_NUM_AUDIO_INDICES + packet->index;
    audio_packet* audio_pkt = &receiving_audio[audio_id % RECV_AUDIO_BUFFER_SIZE];
    if (audio_pkt->id != -1) {
        mprintf("Audio packet being overwritten before being played! ID %d replaced with ID %d, when the Last Played ID was %d\n", audio_pkt->id, audio_id, AudioData.last_played_id);
        AudioData.last_played_id = audio_id - 1;
        for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
            if (receiving_audio[i].id <= AudioData.last_played_id) {
                receiving_audio[i].id = -1;
            }
        }
    }
    audio_pkt->id = audio_id;
    audio_pkt->size = packet->payload_size;
    memcpy(audio_pkt->data, packet->data, packet->payload_size);

    if (packet->is_ending) {
        for (int i = audio_id + 1; i % MAX_NUM_AUDIO_INDICES != 0; i++) {
            receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = i;
            receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].size = 0;
        }
    }

    updateQueuedAudio();

    return 0;
}