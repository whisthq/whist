/*
 * Client audio functions.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#ifndef DESKTOP_AUDIO_H
#define DESKTOP_AUDIO_H

#include "../fractal/audio/audiodecode.h"
#include "../fractal/core/fractal.h"
#include "main.h"

void initAudio();

int32_t ReceiveAudio(struct RTPPacket *packet);

void updateAudio();

void destroyAudio();

#endif  // DESKTOP_AUDIO_H
