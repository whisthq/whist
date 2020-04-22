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

void set_video_active_resizing( bool is_resizing );

#endif // DESKTOP_VIDEO_H
