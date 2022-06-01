extern "C" {
#include "common.h"
#include "native.h"
}

#include "sdl_struct.hpp"

void sdl_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_GL_GetDrawableSize(context->window, width, height);
}

void sdl_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_GetWindowSize(context->window, width, height);
}

WhistStatus sdl_get_window_display_index(WhistFrontend* frontend, int* index) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    int ret = SDL_GetWindowDisplayIndex(context->window);
    if (ret < 0) {
        LOG_ERROR("Could not get window display index - %s", SDL_GetError());
        return WHIST_ERROR_UNKNOWN;
    }
    if (index != NULL) {
        *index = ret;
    }
    return WHIST_SUCCESS;
}

int sdl_get_window_dpi(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    return sdl_native_get_dpi(context->window);
}

bool sdl_is_window_visible(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    return !(SDL_GetWindowFlags(context->window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED));
}

WhistStatus sdl_set_title(WhistFrontend* frontend, const char* title) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .code = SDL_FRONTEND_EVENT_WINDOW_TITLE_CHANGE,
                .data1 = (void*)title,
            },
    };
    SDL_PushEvent(&event);
    return WHIST_SUCCESS;
}

void sdl_restore_window(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_RestoreWindow(context->window);
}

void sdl_resize_window(WhistFrontend* frontend, int width, int height) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_SetWindowSize(context->window, width, height);
}

void sdl_set_titlebar_color(WhistFrontend* frontend, const WhistRGBColor* color) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .code = SDL_FRONTEND_EVENT_TITLE_BAR_COLOR_CHANGE,
                .data1 = (void*)color,
            },
    };
    SDL_PushEvent(&event);
}

void sdl_display_notification(WhistFrontend* frontend, const WhistNotification* notif) {
    sdl_native_display_notification(notif);
}
