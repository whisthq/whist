#ifndef DESKTOP_AUDIO_H
#define DESKTOP_AUDIO_H

#include "../include/fractal.h"
#include "../include/audiodecode.h"
#include "main.h"

void initAudio();
int32_t ReceiveAudio(struct RTPPacket* packet);
void updateAudio();
void destroyAudio();

#endif