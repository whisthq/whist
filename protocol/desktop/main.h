#ifndef DESKTOP_MAIN_H
#define DESKTOP_MAIN_H

#include "../include/fractal.h"

int SendFmsg( struct FractalClientMessage* fmsg );
int SendPacket(void* data, int len);

#endif // DESKTOP_MAIN_H