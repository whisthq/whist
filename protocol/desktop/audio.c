#include "audio.h"

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
}

void destroyAudio() {
    SDL_CloseAudioDevice(AudioData.dev);
    destroy_audio_decoder(AudioData.audio_decoder);
}

int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size) {
    int played_id = packet->id * 1000 + packet->index;
    if (played_id > AudioData.last_played_id) {
        AudioData.last_played_id = played_id;
        SDL_QueueAudio(AudioData.dev, packet->data, packet->payload_size);
    }

    return 0;
}