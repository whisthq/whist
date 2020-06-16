#ifndef PEER_H
#define PEER_H

#include "../fractal/core/fractal.h"
#include "SDL2/SDL.h"

int renderPeers(SDL_Renderer *renderer, PeerUpdateMessage *msgs, size_t num_msgs);

#endif  // PEER_H
