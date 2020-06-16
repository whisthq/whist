#include "../fractal/core/fractal.h"
#include "../fractal/utils/logging.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "SDL2/SDL.h"
#include "peer.h"

extern int client_id;

extern volatile SDL_Window* window;

int renderPeers(SDL_Renderer *renderer, PeerUpdateMessage *msgs, size_t num_msgs) {
    SDL_Rect rect;
    int ret = 0;
    int window_width, window_height;
    SDL_GetWindowSize((SDL_Window *) window, &window_width, &window_height);

    SDL_Surface *cursor_surface = SDL_LoadBMP("../../cursor.bmp");
    if (cursor_surface == NULL) {
        LOG_ERROR("Failed to load cursor bmp. (Error: %s)", SDL_GetError());
        return -1;
    }
    SDL_LockSurface(cursor_surface);
    uint32_t key_color = SDL_MapRGB(cursor_surface->format, 255, 255, 255);
    SDL_SetColorKey(cursor_surface, SDL_TRUE, key_color);
    SDL_UnlockSurface(cursor_surface);
    SDL_Texture *cursor_texture = SDL_CreateTextureFromSurface(renderer, cursor_surface);
    SDL_FreeSurface(cursor_surface);
    if (cursor_texture == NULL) {
        LOG_ERROR("Failed to create cursor texture from surface."
                  " (Error: %s)", SDL_GetError());
        return -1;
    }

    for (; num_msgs > 0; msgs++, num_msgs--) {
        if (client_id == msgs->peer_id) {
            continue;
        }
        rect.w = 25;
        rect.h = 25;
        rect.x = msgs->x * window_width / (int32_t)MOUSE_SCALING_FACTOR;
        rect.y = msgs->y * window_height / (int32_t)MOUSE_SCALING_FACTOR;
        if (SDL_RenderCopy(renderer, cursor_texture, NULL, &rect) != 0) {
            LOG_ERROR("Failed to render copy peer mouse.");
        }
        rect.w = 5;
        rect.h = 5;
        rect.x -= rect.w / 2;
        rect.y -= rect.h / 2;
        SDL_SetRenderDrawColor(renderer, msgs->color.r, msgs->color.g, msgs->color.b, 255);
        if (SDL_RenderFillRect(renderer, &rect) != 0) {
          LOG_ERROR("Failed to render peer rectangle.");
          ret = -1;
        }
        // if (msgs->is_controlling) {
        //
        // }
    }
    SDL_DestroyTexture(cursor_texture);
    return ret;
}
