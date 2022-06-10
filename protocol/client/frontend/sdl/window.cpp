#include <SDL2/SDL_video.h>
#include <whist/core/whist.h>
extern "C" {
#include "common.h"
#include "native.h"
}

#include "sdl_struct.hpp"

WhistStatus sdl_get_window_pixel_size(WhistFrontend* frontend, int id, int* width, int* height) {
    /*
     * Get the actual size (DPI-adjusted) of the window with given id.
     */
#if USING_MULTIWINDOW
    SDL_DisplayMode dm;

    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        LOG_ERROR("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        *width = 1920;
        *height = 1080;
    } else {
        int dpi_scale = sdl_get_dpi_scale(frontend);
        *width = dm.w * dpi_scale;
        *height = dm.h * dpi_scale;
    }
#else
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDL_GL_GetDrawableSize(context->windows[id]->window, width, height);
        return WHIST_SUCCESS;
    }
    LOG_ERROR("Tried to get window pixel size for window %d, but no such window exists!", id);
    if (width != NULL) {
        *width = WHIST_ERROR_NOT_FOUND;
    }
    if (height != NULL) {
        *height = WHIST_ERROR_NOT_FOUND;
    }
    return WHIST_ERROR_NOT_FOUND;
#endif
}

WhistStatus sdl_get_window_virtual_size(WhistFrontend* frontend, int id, int* width, int* height) {
    /*
     * Get the non-DPI-adjusted width and height of the window with given id. This will be a
     * fraction of the actual size in high DPI monitors.
     */
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (context->windows.contains(id)) {
        SDL_GetWindowSize(context->windows[id]->window, width, height);
        return WHIST_SUCCESS;
    }
    LOG_ERROR("Tried to get window virtual size for window %d, but no such window exists!", id);
    if (width != NULL) {
        *width = WHIST_ERROR_NOT_FOUND;
    }
    if (height != NULL) {
        *height = WHIST_ERROR_NOT_FOUND;
    }
    return WHIST_ERROR_NOT_FOUND;
}

// TODO: not window-specific for now. But we should make it so soon

WhistStatus sdl_get_window_display_index(WhistFrontend* frontend, int* index) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (!context->windows.empty() || context->windows.begin()->second->window == NULL) {
        int ret = SDL_GetWindowDisplayIndex(context->windows.begin()->second->window);
        if (ret < 0) {
            // LOG_ERROR("Could not get window display index - %s", SDL_GetError());
            return WHIST_ERROR_UNKNOWN;
        }
        if (index != NULL) {
            *index = ret;
        }
        return WHIST_SUCCESS;
    }
    LOG_ERROR("Tried to get window display index for window %d, but no such window exists!", id);
    if (index != NULL) {
        *index = WHIST_ERROR_NOT_FOUND;
    }
    return WHIST_ERROR_NOT_FOUND;
}

int sdl_get_window_dpi(WhistFrontend* frontend) {
    // TODO: Doesn't support different monitors with different DPI's
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (!context->windows.empty() || context->windows.begin()->second->window == NULL) {
        return sdl_native_get_dpi(context->windows.begin()->second->window);
    } else {
        LOG_ERROR("Could not get DPI! No windows available");
        return 96;
    }
}

// Declared in sdl_struct.hpp
int sdl_get_dpi_scale(WhistFrontend* frontend) {
    // TODO: Doesn't support different monitors with different DPI's
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    if (!context->windows.empty() || context->windows.begin()->second->window == NULL) {
        int pixel_width, virtual_width;
        SDL_GetWindowSize(context->windows.begin()->second->window, &virtual_width, NULL);
        SDL_GL_GetDrawableSize(context->windows.begin()->second->window, &pixel_width, NULL);
        return pixel_width / virtual_width;
    } else {
        LOG_ERROR("Could not get DPI scale! No windows available");
        return 1;
    }
}

// Checks if at least one window is visible. Returns false if all windows are minimized or occluded.
bool sdl_is_any_window_visible(WhistFrontend* frontend) {
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
#if !USING_MULTIWINDOW
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .timestamp = 0,
                .code = SDL_FRONTEND_EVENT_WINDOW_TITLE_CHANGE,
                .data1 = (void*)title,
                // Cast the data directly into the 8bytes of void*,
                // So data2 holds the id directly, rather than a pointer to it
                .data2 = (void*)(intptr_t)id,
            },
    };
    // NOTE: "title" will be freed when the event is consumed
    SDL_PushEvent(&event);
#endif
    return WHIST_SUCCESS;
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
#if !USING_MULTIWINDOW
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .timestamp = 0,
                .code = SDL_FRONTEND_EVENT_TITLE_BAR_COLOR_CHANGE,
                .data1 = (void*)color,
                // Cast the data directly into the 8bytes of void*,
                // So data2 holds the id directly, rather than a pointer to it
                .data2 = (void*)(intptr_t)id,
            },
    };
    // NOTE: "color" will be freed when the event is consumed
    SDL_PushEvent(&event);
#endif
}

void sdl_display_notification(WhistFrontend* frontend, const WhistNotification* notif) {
    sdl_native_display_notification(notif);
}

const char* sdl_get_chosen_file(WhistFrontend* frontend) { return sdl_native_get_chosen_file(); }

void* sdl_file_download_start(WhistFrontend* frontend, const char* file_path, int64_t file_size) {
    return (void*)file_path;
}

void sdl_file_download_update(WhistFrontend* frontend, void* opaque, int64_t bytes_so_far,
                              int64_t bytes_per_sec) {}

void sdl_file_download_complete(WhistFrontend* frontend, void* opaque) {
    sdl_native_file_download_notify_finished((const char*)opaque);
}
