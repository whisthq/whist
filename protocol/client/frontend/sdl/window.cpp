#include <whist/core/whist.h>
extern "C" {
#include "common.h"
#include "native.h"
}

#include "sdl_struct.hpp"

void sdl_get_window_pixel_size(WhistFrontend* frontend, int id, int* width, int* height) {
    /*
     * Get the actual size (DPI-adjusted) of the window with given id.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDL_GL_GetDrawableSize(context->windows[id]->window, width, height);
    } else {
        LOG_ERROR("Tried to get window pixel size for window %d, but no such window exists!", id);
        if (width != NULL) {
            *width = WHIST_ERROR_NOT_FOUND;
        }
        if (height != NULL) {
            *height = WHIST_ERROR_NOT_FOUND;
        }
    }
}

void sdl_get_window_virtual_size(WhistFrontend* frontend, int id, int* width, int* height) {
    /*
     * Get the non-DPI-adjusted width and height of the window with given id. This will be a
     * fraction of the actual size in high DPI monitors.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDL_GetWindowSize(context->windows[id]->window, width, height);
    } else {
        LOG_ERROR("Tried to get window virtual size for window %d, but no such window exists!", id);
        if (width != NULL) {
            *width = WHIST_ERROR_NOT_FOUND;
        }
        if (height != NULL) {
            *height = WHIST_ERROR_NOT_FOUND;
        }
    }
}

WhistStatus sdl_get_window_display_index(WhistFrontend* frontend, int id, int* index) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        int ret = SDL_GetWindowDisplayIndex(context->windows[id]->window);
        if (ret < 0) {
            // LOG_ERROR("Could not get window display index - %s", SDL_GetError());
            return WHIST_ERROR_UNKNOWN;
        }
        if (index != NULL) {
            *index = ret;
        }
        return WHIST_SUCCESS;
    } else {
        LOG_ERROR("Tried to get window display index for window %d, but no such window exists!",
                  id);
        if (width != NULL) {
            *width = WHIST_ERROR_NOT_FOUND;
        }
        if (height != NULL) {
            *height = WHIST_ERROR_NOT_FOUND;
        }
        return WHIST_ERROR_NOT_FOUND;
    }
}

int sdl_get_window_dpi(WhistFrontend* frontend) {
    // TODO: changed to only check one window. But in tehory there could be multiple windows on
    // different DPI monitors, I guess.
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (!context->windows.empty()) {
        return sdl_native_get_dpi(context->windows.begin()->second->window);
    }
    return -1;
}

bool sdl_is_window_visible(WhistFrontend* frontend) {
    // TODO: changed to check if any window is visible
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    for (const auto& [window_id, window_context] : context->windows) {
        if (!(SDL_GetWindowFlags(window_context->window) &
              (SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED))) {
            return true;
        }
    }
    return false;
}

WhistStatus sdl_set_title(WhistFrontend* frontend, int id, const char* title) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .timestamp = 0,
                .code = SDL_FRONTEND_EVENT_WINDOW_TITLE_CHANGE,
                .data1 = (void*)title,
                .data2 = (void*)(intptr_t)id,
            },
    };
    // NOTE: "title" will be freed when the event is consumed
    SDL_PushEvent(&event);
}

void sdl_restore_window(WhistFrontend* frontend, int id) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDL_RestoreWindow(context->windows[id]->window);
    } else {
        LOG_ERROR("Tried to restore window %d, but no such window exists!", id);
    }
}

void sdl_resize_window(WhistFrontend* frontend, int id, int width, int height) {
    /*
     * Resize the window with given id. Width and height are given in virtual pixels, not actual
     * pixels.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDL_SetWindowSize(context->windows[id]->window, width, height);
    } else {
        LOG_ERROR("Tried to resize window %d, but no such window exists!", id);
    }
}

void sdl_set_titlebar_color(WhistFrontend* frontend, int id, const WhistRGBColor* color) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .timestamp = 0,
                .code = SDL_FRONTEND_EVENT_TITLE_BAR_COLOR_CHANGE,
                .data1 = (void*)color,
                .data2 = (void*)(intptr_t)id,
            },
    };
    // NOTE: "color" will be freed when the event is consumed
    SDL_PushEvent(&event);
}

void sdl_display_notification(WhistFrontend* frontend, const WhistNotification* notif) {
    sdl_native_display_notification(notif);
}
