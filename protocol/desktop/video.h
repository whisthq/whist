/*
 * General client video functions.
 * 
 * Copyright Fractal Computers, Inc. 2020
**/
#ifndef DESKTOP_VIDEO_H
#define DESKTOP_VIDEO_H

#include "../include/fractal.h"
#include "../include/videodecode.h"
#include "main.h"

void initVideo();

int32_t ReceiveVideo(struct RTPPacket *packet);

void updateVideo();

void destroyVideo();

void notify_video(bool stop);

#endif // DESKTOP_VIDEO_H
