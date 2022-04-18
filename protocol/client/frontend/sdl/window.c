#include "common.h"
#include "native.h"

void sdl_get_window_pixel_size(WhistFrontend* frontend, int* width, int* height) {
    SDLFrontendContext* context = frontend->context;
    SDL_GL_GetDrawableSize(context->window, width, height);
}

void sdl_get_window_virtual_size(WhistFrontend* frontend, int* width, int* height) {
    SDLFrontendContext* context = frontend->context;
    SDL_GetWindowSize(context->window, width, height);
}

void sdl_get_window_position(WhistFrontend* frontend, int* x, int* y) {
    SDLFrontendContext* context = frontend->context;
    SDL_GetWindowPosition(context->window, x, y);
}

WhistStatus sdl_get_window_display_index(WhistFrontend* frontend, int* index) {
    SDLFrontendContext* context = frontend->context;
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
    SDLFrontendContext* context = frontend->context;
    return sdl_native_get_dpi(context->window);
}

bool sdl_is_window_visible(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    return !(SDL_GetWindowFlags(context->window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED));
}

WhistStatus sdl_set_title(WhistFrontend* frontend, const char* title) {
    SDLFrontendContext* context = frontend->context;
    if (context->window == NULL) {
        return WHIST_ERROR_NOT_FOUND;
    }
    SDL_SetWindowTitle(context->window, title);
    return WHIST_SUCCESS;
}

void sdl_restore_window(WhistFrontend* frontend) {
    SDLFrontendContext* context = frontend->context;
    SDL_RestoreWindow(context->window);
}

void sdl_resize_window(WhistFrontend* frontend, int width, int height) {
    SDLFrontendContext* context = frontend->context;
    SDL_SetWindowSize(context->window, width, height);
}

void sdl_set_titlebar_color(WhistFrontend* frontend, const WhistRGBColor* color) {
    SDLFrontendContext* context = frontend->context;
    sdl_native_set_titlebar_color(context->window, color);
}
