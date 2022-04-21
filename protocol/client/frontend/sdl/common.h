#ifndef WHIST_CLIENT_FRONTEND_SDL_COMMON_H
#define WHIST_CLIENT_FRONTEND_SDL_COMMON_H

#define SDL_MAIN_HANDLED
// So that SDL sees symbols such as memcpy
#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#endif
#include <SDL2/SDL.h>
#include <whist/core/whist.h>
#include "../api.h"
#include "../frontend.h"

typedef struct SDLFrontendContext {
    SDL_AudioDeviceID audio_device;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    struct {
        WhistCursorState state;
        struct {
            int x;
            int y;
        } last_visible_position;
        uint32_t hash;
        SDL_Cursor* handle;
    } cursor;
    const uint8_t* key_state;
    int key_count;
    uint32_t file_drag_event_id;
    void* file_drag_data;
} SDLFrontendContext;

#define SDL_COMMON_HEADER_ENTRY(return_type, name, ...) return_type sdl_##name(__VA_ARGS__);
FRONTEND_API(SDL_COMMON_HEADER_ENTRY)
#undef SDL_COMMON_HEADER_ENTRY

#endif  // WHIST_CLIENT_FRONTEND_SDL_COMMON_H
