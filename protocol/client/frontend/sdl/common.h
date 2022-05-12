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
    /**
     * Fullscreen event.
     *
     * data1 contains a boolean cast to a pointer indicating whether we
     * want to enter (true) or exit (false) fullscreen mode.
     */
    SDL_FRONTEND_EVENT_FULLSCREEN,
    /**
     * Window title change event.
     *
     * data1 contains a pointer to a string, which the receiver will
     * need to free after tuse.
     */
    SDL_FRONTEND_EVENT_WINDOW_TITLE_CHANGE,
    /**
     * Window title bar colour change.
     *
     * data1 contains a pointer to a WhistRGBColor object, which the
     * receiver will need to free after use.
     */
    SDL_FRONTEND_EVENT_TITLE_BAR_COLOR_CHANGE,
    /**
     * Internal interrupt from whist_frontend_interrupt().
     */
    SDL_FRONTEND_EVENT_INTERRUPT,
};

#endif  // WHIST_CLIENT_FRONTEND_SDL_COMMON_H
