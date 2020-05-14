#include "../fractal/core/fractal.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "video.h"

SDL_Window* initSDL(int output_width, int output_height);
void destroySDL(SDL_Window* window);
