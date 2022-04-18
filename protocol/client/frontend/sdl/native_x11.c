#include "native.h"

// Already handled by SDL
void sdl_native_hide_taskbar(void) {}

int sdl_native_set_titlebar_color(SDL_Window* window, const WhistRGBColor* color) {
    LOG_INFO("Not implemented on X11.");
    return 0;
}

void sdl_native_init_window_options(SDL_Window* window) {}

int sdl_native_get_dpi(SDL_Window* window) {
    /*
        Get the DPI for the display of the provided window.

        Arguments:
            window (SDL_Window*):       SDL window whose DPI to retrieve.

        Returns:
            (int): The DPI as an int, with 96 as a base.
    */

    // We just fall back to the SDL implementation here.
    int display_index = SDL_GetWindowDisplayIndex(window);
    float dpi;
    SDL_GetDisplayDPI(display_index, NULL, &dpi, NULL);
    return (int)dpi;
}

void sdl_native_declare_user_activity(void) {
    // TODO: Implement actual wake-up code
    // For now, we'll just disable the screensaver
    SDL_DisableScreenSaver();
}

void sdl_native_init_external_drag_handler(void) { LOG_INFO("Not implemented on X11"); }

void sdl_native_destroy_external_drag_handler(void) { LOG_INFO("Not implemented on X11"); }
