/**
 * Copyright Fractal Computers, Inc. 2020
 * @file sdl_utils.c
 * @brief This file contains the code to create and destroy SDL windows on the
 *        client.
============================
Usage
============================

initSDL gets called first to create an SDL window, and destroySDL at the end to
close the window.
*/

/*
============================
Includes
============================
*/

#include "sdl_utils.h"
#include <fractal/utils/png.h>
#include <fractal/utils/lodepng.h>

#include <fractal/utils/color.h>
#include "native_window_utils.h"

extern volatile int output_width;
extern volatile int output_height;
extern volatile SDL_Window* window;
bool skip_taskbar = false;

#if defined(_WIN32)
HHOOK g_h_keyboard_hook;
LRESULT CALLBACK low_level_keyboard_proc(INT n_code, WPARAM w_param, LPARAM l_param);
#endif

#ifdef __APPLE__
// on macOS, we must initialize the renderer in `init_sdl()` instead of video.c
volatile SDL_Renderer* init_sdl_renderer;
#endif

/*
============================
Private Function Implementations
============================
*/

void send_captured_key(SDL_Keycode key, int type, int time) {
    /*
        Send a key to SDL event queue, presumably one that is captured and wouldn't
        naturally make it to the event queue by itself

        Arguments:
            key (SDL_Keycode): key that was captured
            type (int): event type (press or release)
            time (int): time that the key event was registered
    */

    SDL_Event e = {0};
    e.type = type;
    e.key.keysym.sym = key;
    e.key.keysym.scancode = SDL_GetScancodeFromName(SDL_GetKeyName(key));
    LOG_INFO("KEY: %d", key);
    e.key.timestamp = time;
    SDL_PushEvent(&e);
}

int resizing_event_watcher(void* data, SDL_Event* event) {
    /*
        Event watcher to be used in SDL_AddEventWatch to capture
        and handle window resize events

        Arguments:
            data (void*): SDL Window data
            event (SDL_Event*): SDL event to be analyzed

        Return:
            (int): 0 on success
    */

    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED) {
        // If the resize event if for the current window
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window*)data) {
            // Notify video.c about the active resizing
            set_video_active_resizing(true);
        }
    }
    return 0;
}

void set_window_icon_from_png(SDL_Window* sdl_window, char* filename) {
    /*
        Set the icon for a SDL window from a PNG file

        Arguments:
            sdl_window (SDL_Window*): The window whose icon will be set
            filename (char*): A path to a `.png` file containing the 64x64 pixel icon.

        Return:
            None
    */

    SDL_Surface* icon_surface = sdl_surface_from_png_file(filename);
    SDL_SetWindowIcon(sdl_window, icon_surface);
    free_sdl_rgb_surface(icon_surface);
}

SDL_Window* init_sdl(int target_output_width, int target_output_height, char* name,
                     char* icon_filename) {
    /*
        Attaches the current thread to the specified current input client

        Arguments:
            target_output_width (int): The width of the SDL window to create, in pixels
            target_output_height (int): The height of the SDL window to create, in pixels
            name (char*): The title of the window
            icon_filename (char*): The filename of the window icon, pointing to a 64x64 png,
                or "" for the default icon

        Return:
            sdl_window (SDL_Window*): NULL if fails to create SDL window, else the created SDL
                window
    */

#if defined(_WIN32)
    // set Windows DPI
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif

#if defined(_WIN32) && CAPTURE_SPECIAL_WINDOWS_KEYS
    // Hook onto windows keyboard to intercept windows special key combinations
    g_hKeyboardHook =
        SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        LOG_ERROR("Could not initialize SDL - %s", SDL_GetError());
        return NULL;
    }

    // TODO: make this a commandline argument based on client app settings!
    int full_width = get_virtual_screen_width();
    int full_height = get_virtual_screen_height();

    bool maximized = target_output_width == 0 && target_output_height == 0;

    // Default output dimensions will be a quarter of the full screen if the window
    // starts maximized. Even if this isn't a multiple of 8, it's fine because
    // clicking the minimize button will trigger an SDL resize event
    if (target_output_width == 0) {
        target_output_width = full_width / 2;
    }

    if (target_output_height == 0) {
        target_output_height = full_height / 2;
    }

    SDL_Window* sdl_window;

    if (skip_taskbar) {
        hide_native_window_taskbar();
    }

    const uint32_t window_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL |
                                  SDL_WINDOW_RESIZABLE | (maximized ? SDL_WINDOW_MAXIMIZED : 0) |
                                  (skip_taskbar ? SDL_WINDOW_SKIP_TASKBAR : 0);

    // Simulate fullscreen with borderless always on top, so that it can still
    // be used with multiple monitors
    sdl_window = SDL_CreateWindow((name == NULL ? "Fractal" : name), SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, target_output_width, target_output_height,
                                  window_flags);
    if (!sdl_window) {
        LOG_ERROR("SDL: could not create window - exiting: %s", SDL_GetError());
        return NULL;
    }

/*
    On macOS, we must initialize the renderer in the main thread -- seems not needed
    and not possible on other OSes. If the renderer is created later in the main thread
    on macOS, the user will see a window open in this function, then close/reopen during
    renderer creation
*/
#ifdef __APPLE__
    init_sdl_renderer =
        SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
#endif

    // Set icon
    if (strcmp(icon_filename, "") != 0) {
        set_window_icon_from_png(sdl_window, icon_filename);
    }

    const FractalRGBColor white = {255, 255, 255};
    set_native_window_color(sdl_window, white);

    SDL_Event cur_event;
    while (SDL_PollEvent(&cur_event)) {
        // spin to clear SDL event queue
        // this effectively waits for window load on Mac
    }

    // After creating the window, we will grab DPI-adjusted dimensions in real
    // pixels
    output_width = get_window_pixel_width((SDL_Window*)sdl_window);
    output_height = get_window_pixel_height((SDL_Window*)sdl_window);

    return sdl_window;
}

void destroy_sdl(SDL_Window* window_param) {
    /*
        Destroy the SDL resources

        Arguments:
            window_param (SDL_Window*): SDL window to be destroyed
    */

    LOG_INFO("Destroying SDL");
#if defined(_WIN32)
    UnhookWindowsHookEx(g_h_keyboard_hook);
#endif
    if (window_param) {
        SDL_DestroyWindow((SDL_Window*)window_param);
        window_param = NULL;
    }
    SDL_Quit();
}

#if defined(_WIN32)
HHOOK mule;
LRESULT CALLBACK low_level_keyboard_proc(INT n_code, WPARAM w_param, LPARAM l_param) {
    /*
        Function to capture keyboard strokes and block them if they encode special
        key combinations, with intent to redirect them to send_captured_key so that the
        keys can still be streamed over to the host

        Arguments:
            n_code (INT): keyboard code
            w_param (WPARAM): w_param to be passed to CallNextHookEx
            l_param (LPARAM): l_param to be passed to CallNextHookEx

        Return:
            (LRESULT CALLBACK): CallNextHookEx return callback value
    */

    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)l_param;
    int flags = SDL_GetWindowFlags((SDL_Window*)window);
    if ((flags & SDL_WINDOW_INPUT_FOCUS)) {
        switch (n_code) {
            case HC_ACTION: {
                // Check to see if the CTRL key is pressed
                BOOL b_control_key_down = GetAsyncKeyState(VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);
                BOOL b_alt_key_down = pkbhs->flags & LLKHF_ALTDOWN;

                int type = (pkbhs->flags & LLKHF_UP) ? SDL_KEYUP : SDL_KEYDOWN;
                int time = pkbhs->time;

                // Disable LWIN
                if (pkbhs->vkCode == VK_LWIN) {
                    send_captured_key(SDLK_LGUI, type, time);
                    return 1;
                }

                // Disable RWIN
                if (pkbhs->vkCode == VK_RWIN) {
                    send_captured_key(SDLK_RGUI, type, time);
                    return 1;
                }

                // Disable CTRL+ESC
                if (pkbhs->vkCode == VK_ESCAPE && b_control_key_down) {
                    send_captured_key(SDLK_ESCAPE, type, time);
                    return 1;
                }

                // Disable ALT+ESC
                if (pkbhs->vkCode == VK_ESCAPE && b_alt_key_down) {
                    send_captured_key(SDLK_ESCAPE, type, time);
                    return 1;
                }

                // Disable ALT+TAB
                if (pkbhs->vkCode == VK_TAB && b_alt_key_down) {
                    send_captured_key(SDLK_TAB, type, time);
                    return 1;
                }

                // Disable ALT+F4
                if (pkbhs->vkCode == VK_F4 && b_alt_key_down) {
                    send_captured_key(SDLK_F4, type, time);
                    return 1;
                }

                break;
            }
            default:
                break;
        }
    }
    return CallNextHookEx(mule, n_code, w_param, l_param);
}
#endif

// masks for little-endian RGBA
#define RGBA_MASK_A 0xff000000
#define RGBA_MASK_B 0x00ff0000
#define RGBA_MASK_G 0x0000ff00
#define RGBA_MASK_R 0x000000ff

SDL_Surface* sdl_surface_from_png_file(char* filename) {
    /*
        Load a PNG file to an SDL surface using lodepng.

        Arguments:
            filename (char*): PNG image file path

        Returns:
            surface (SDL_Surface*): the loaded surface on success, and NULL on failure

        NOTE:
            After a successful call to sdl_surface_from_png_file, remember to call
            `SDL_FreeSurface(surface)` to free memory.
    */

    unsigned int w, h, error;
    unsigned char* image;

    // decode to 32-bit RGBA
    // TODO: We need to free image after destroying the SDL_Surface!
    // Perhaps we should simply return a PNG buffer and have the caller
    // manage memory allocation.
    error = lodepng_decode32_file(&image, &w, &h, filename);
    if (error) {
        LOG_ERROR("decoder error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    // buffer pointer, width, height, bits per pixel, bytes per row, R/G/B/A masks
    SDL_Surface* surface =
        SDL_CreateRGBSurfaceFrom(image, w, h, sizeof(uint32_t) * 8, sizeof(uint32_t) * w,
                                 RGBA_MASK_R, RGBA_MASK_G, RGBA_MASK_B, RGBA_MASK_A);

    if (surface == NULL) {
        LOG_ERROR("Failed to load SDL surface from file '%s': %s", filename, SDL_GetError());
        free(image);
        return NULL;
    }

    return surface;
}

void free_sdl_rgb_surface(SDL_Surface* surface) {
    // Specified to call `SDL_FreeSurface` on the surface, and `free` on the pixels array
    char* pixels = surface->pixels;
    SDL_FreeSurface(surface);
    free(pixels);
}

SDL_mutex* safe_SDL_CreateMutex() {  // NOLINT(readability-identifier-naming)
    SDL_mutex* ret = SDL_CreateMutex();
    if (ret == NULL) {
        LOG_FATAL("Failed to safe_SDL_CreateMutex! %s", SDL_GetError());
    }
    return ret;
}

void safe_SDL_LockMutex(SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    if (SDL_LockMutex(mutex) < 0) {
        LOG_FATAL("Failed to safe_SDL_LockMutex! %s", SDL_GetError());
    }
}

int safe_SDL_TryLockMutex(SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    int status = SDL_TryLockMutex(mutex);
    if (status == 0 || status == SDL_MUTEX_TIMEDOUT) {
        return status;
    } else {
        LOG_FATAL("Failed to safe_SDL_LockMutex! %s", SDL_GetError());
    }
}

void safe_SDL_UnlockMutex(SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    if (SDL_UnlockMutex(mutex) < 0) {
        LOG_FATAL("Failed to safe_SDL_UnlockMutex! %s", SDL_GetError());
    }
}

void safe_SDL_CondWait(SDL_cond* cond, SDL_mutex* mutex) {  // NOLINT(readability-identifier-naming)
    if (SDL_CondWait(cond, mutex) < 0) {
        LOG_FATAL("Failed to safe_SDL_CondWait! %s", SDL_GetError());
    }
}
