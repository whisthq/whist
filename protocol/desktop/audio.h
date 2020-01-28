#ifndef DESKTOP_AUDIO_H
#define DESKTOP_AUDIO_H

#include "../include/fractal.h"
#include "../include/audiodecode.h"

void initAudio();
int32_t ReceiveAudio(struct RTPPacket* packet, int recv_size);
void destroyAudio();

#endif