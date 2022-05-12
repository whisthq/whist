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

#define SDL_COMMON_HEADER_ENTRY(return_type, name, ...) return_type sdl_##name(__VA_ARGS__);
FRONTEND_API(SDL_COMMON_HEADER_ENTRY)
#undef SDL_COMMON_HEADER_ENTRY

/**
 * These events are generated and consumed internally by the SDL
 * frontend, typically to manage thread-safety.
 *
 * All of these events are transmitted using the internal_event_id type.
 * This event identifier is placed in the SDL_UserEvent.code field.
 */
enum {
    /**
     * File drag event.
     *
     * data1 contains a pointer to a FrontendFileDragEvent, which the
     * receiver will need to free after use.
     */
    SDL_FRONTEND_EVENT_FILE_DRAG,
};

#endif  // WHIST_CLIENT_FRONTEND_SDL_COMMON_H
