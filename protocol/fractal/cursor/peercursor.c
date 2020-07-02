#include "../core/fractal.h"
#include "../utils/logging.h"
#include "peercursor.h"

static SDL_Surface *base_cursor_surface;

static void replaceColor(SDL_Surface *sfc, Uint32 old_color, Uint32 new_color) {
    for (int x = 0; x < sfc->w; x++) {
        for (int y = 0; y < sfc->h; y++) {
            Uint8 *pixel = (Uint8 *)sfc->pixels;
            pixel += (y * sfc->pitch) + (x * sfc->format->BytesPerPixel);
            if (*((Uint32 *)pixel) == old_color) {
                *((Uint32 *)pixel) = new_color;
            }
        }
    }
}

static SDL_Surface *duplicateSurface(SDL_Surface *surface) {
    if (surface == NULL) {
        LOG_ERROR("surface is null");
        exit(1);
    }
    SDL_Surface *new_surface = SDL_CreateRGBSurface(
        0, surface->w, surface->h, surface->format->BitsPerPixel,
        surface->format->Rmask, surface->format->Gmask, surface->format->Bmask,
        surface->format->Amask);
    if (new_surface == NULL) {
        LOG_ERROR(
            "Failed to create new surface. "
            "(Error: %s)",
            SDL_GetError());
        return NULL;
    }
    if (SDL_BlitSurface(surface, NULL, new_surface, NULL) != 0) {
        LOG_ERROR(
            "Failed to blit surface. "
            "(Error: %s)",
            SDL_GetError());
        SDL_FreeSurface(new_surface);
        return NULL;
    }
    return new_surface;
}

int InitPeerCursors(void) {
    base_cursor_surface = SDL_LoadBMP("../../../fractal/cursor/cursor.bmp");
    if (base_cursor_surface == NULL) {
        LOG_ERROR("Failed to load cursor bmp. (Error: %s)", SDL_GetError());
        return -1;
    }
    if (SDL_LockSurface(base_cursor_surface) != 0) {
        LOG_ERROR(
            "Failed to unlock cursor surface. "
            "(Error: %s)",
            SDL_GetError());
        SDL_FreeSurface(base_cursor_surface);
        return -1;
    }
    if (base_cursor_surface->format->BytesPerPixel != 4) {
        LOG_ERROR("Cursor image is not 32-bit RGB.");
        SDL_FreeSurface(base_cursor_surface);
        return -1;
    }
    Uint32 white = SDL_MapRGB(base_cursor_surface->format, 255, 255, 255);
    if (SDL_SetColorKey(base_cursor_surface, SDL_TRUE, white) != 0) {
        LOG_ERROR(
            "Failed to set cursor's key color. "
            "(Error: %s)",
            SDL_GetError());
        SDL_FreeSurface(base_cursor_surface);
        return -1;
    }
    SDL_UnlockSurface(base_cursor_surface);
    return 0;
}

int DestroyPeerCursors(void) {
    SDL_FreeSurface(base_cursor_surface);
    return 0;
}

int drawPeerCursor(SDL_Renderer *renderer, int x, int y, int r, int g, int b) {
    SDL_Surface *sfc = duplicateSurface(base_cursor_surface);
    if (sfc == NULL) {
        LOG_ERROR("Failed to duplicate base cursor surface.");
        return -1;
    }
    if (SDL_LockSurface(sfc) != 0) {
        LOG_ERROR(
            "Failed to create lock cursor surface. "
            "(Error: %s)",
            SDL_GetError());
        SDL_FreeSurface(sfc);
        return -1;
    }

    Uint32 black = SDL_MapRGB(sfc->format, 0, 0, 0);
    Uint32 cursor_color = SDL_MapRGB(sfc->format, (Uint8)r, (Uint8)g, (Uint8)b);
    replaceColor(sfc, black, cursor_color);

    SDL_Texture *txtr = SDL_CreateTextureFromSurface(renderer, sfc);
    SDL_FreeSurface(sfc);
    if (txtr == NULL) {
        LOG_ERROR(
            "Failed to create cursor texture from surface. "
            "(Error: %s)",
            SDL_GetError());
        return -1;
    }

    SDL_Rect rect;
    rect.w = 25;
    rect.h = 25;
    rect.x = x;
    rect.y = y;

    if (SDL_RenderCopy(renderer, txtr, NULL, &rect) != 0) {
        LOG_ERROR(
            "Failed to render copy cursor. "
            "(Error: %s)",
            SDL_GetError());
        SDL_DestroyTexture(txtr);
        return -1;
    }
    SDL_DestroyTexture(txtr);
    return 0;
}
