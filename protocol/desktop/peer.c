#include "../fractal/core/fractal.h"
#include "../fractal/utils/logging.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "SDL2/SDL.h"
#include "peer.h"

extern int client_id;

extern volatile SDL_Window* window;

void replaceColor(SDL_Surface *sfc, Uint32 old_color, Uint32 new_color) {
    for (int x = 0; x < sfc->w; x++) {
        for (int y = 0; y < sfc->h; y++) {
            Uint8 * pixel = (Uint8*) sfc->pixels;
            pixel += (y * sfc->pitch) + (x * sfc->format->BytesPerPixel);
            if (*((Uint32*)pixel) == old_color) {
                *((Uint32*)pixel) = new_color;
            }
        }
    }
}

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
    if (cursor_surface->format->BytesPerPixel != 4) {
        LOG_ERROR("Cursor image is not 32-bit RGB.");
        SDL_FreeSurface(cursor_surface);
        return -1;
    }
    uint32_t black = SDL_MapRGB(cursor_surface->format, 0, 0, 0);
    uint32_t white = SDL_MapRGB(cursor_surface->format, 255, 255, 255);
    SDL_SetColorKey(cursor_surface, SDL_TRUE, white);
    SDL_UnlockSurface(cursor_surface);

    for (; num_msgs > 0; msgs++, num_msgs--) {
        if (client_id == msgs->peer_id) {
            continue;
        }

        SDL_Surface *temp_surface = SDL_CreateRGBSurface(0,
            cursor_surface->w, cursor_surface->h,
            cursor_surface->format->BitsPerPixel,
            cursor_surface->format->Rmask,
            cursor_surface->format->Gmask,
            cursor_surface->format->Bmask,
            cursor_surface->format->Amask);
        if (SDL_BlitSurface(cursor_surface, NULL, temp_surface, NULL) != 0) {
            LOG_ERROR("Failed to blit cursor surface "
                      "(Error: %s)", SDL_GetError());
            SDL_FreeSurface(temp_surface);
            continue;
        }
        SDL_LockSurface(temp_surface);
        uint32_t color = SDL_MapRGB(temp_surface->format,
                                msgs->color.r, msgs->color.g, msgs->color.b);
        replaceColor(temp_surface, black, color);
        SDL_Texture *temp_texture = SDL_CreateTextureFromSurface(renderer,
                                                temp_surface);
        SDL_FreeSurface(temp_surface);
        if (temp_texture == NULL) {
            LOG_ERROR("Failed to create cursor texture from surface."
                      " (Error: %s)", SDL_GetError());
            continue;
        }
        rect.w = 25;
        rect.h = 25;
        rect.x = msgs->x * window_width / (int32_t)MOUSE_SCALING_FACTOR;
        rect.y = msgs->y * window_height / (int32_t)MOUSE_SCALING_FACTOR;
        if (SDL_RenderCopy(renderer, temp_texture, NULL, &rect) != 0) {
            LOG_ERROR("Failed to render copy peer mouse.");
        }
        SDL_DestroyTexture(temp_texture);

        if (msgs->is_controlling) {
            rect.x = msgs->x * window_width / (int32_t)MOUSE_SCALING_FACTOR;
            rect.y = msgs->y * window_height / (int32_t)MOUSE_SCALING_FACTOR;
            rect.w = 5;
            rect.h = 5;
            rect.x -= rect.w / 2;
            rect.y -= rect.h / 2;
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            if (SDL_RenderFillRect(renderer, &rect) != 0) {
              LOG_ERROR("Failed to render peer rectangle.");
              ret = -1;
            }
        }
    }
    SDL_FreeSurface(cursor_surface);
    return ret;
}
