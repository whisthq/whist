#ifndef DESKTOP_VIDEO_H
#define DESKTOP_VIDEO_H

#include "../include/fractal.h"
#include "../include/videodecode.h"
#include "main.h"

void initVideo();
int32_t ReceiveVideo(struct RTPPacket* packet, int recv_size);
void updateVideo();
void destroyVideo();

#endif
