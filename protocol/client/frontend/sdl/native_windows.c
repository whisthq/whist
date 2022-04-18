#include "native.h"
#include <SDL2/SDL_syswm.h>

static HWND get_native_window(SDL_Window* window) {
    SDL_SysWMinfo sys_info = {0};
    SDL_GetWindowWMInfo(window, &sys_info);
    return sys_info.info.win.window;
}

// Already handled by SDL
void sdl_native_hide_taskbar(void) {}

void sdl_native_init_window_options(SDL_Window* window) {}

int sdl_native_set_titlebar_color(SDL_Window* window, const WhistRGBColor* color) {
    LOG_INFO("Not implemented on Windows.");
    return 0;
}

int sdl_native_get_dpi(SDL_Window* window) {
    HWND native_window = get_native_window(window);
    return (int)GetDpiForWindow(native_window);
}

void sdl_native_declare_user_activity(void) {
    // TODO: Implement actual wake-up code
    // For now, we'll just disable the screensaver
    SDL_DisableScreenSaver();
}

void sdl_native_init_external_drag_handler(void) { LOG_INFO("Not implemented on Windows"); }

void sdl_native_destroy_external_drag_handler(void) { LOG_INFO("Not implemented on Windows"); }
