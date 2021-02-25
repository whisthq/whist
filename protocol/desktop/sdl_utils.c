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
#ifdef _WIN32
#include "SDL_syswm.h"
#else
#include <sys/un.h>
#endif

#include "sdl_utils.h"
#include "utils/png.h"
#include "utils/string_utils.h"

#if CAN_UPDATE_WINDOW_TITLEBAR_COLOR
#include "utils/color.h"
#include "native_window_utils.h"
#endif  // CAN_UPDATE_WINDOW_TITLEBAR_COLOR

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
volatile SDL_Renderer* renderer;
#endif

/*
============================
Custom Types
============================
*/

// Hold information to pass onto the window control event watcher
//     This is used for socket/named pipe communication to the client app
typedef struct WindowControlArguments {
    SDL_Window* window;
#ifdef _WIN32
    HANDLE read_socket_fd;
    HANDLE write_socket_fd;
#else  // UNIX
    int write_socket_fd;
#endif
} WindowControlArguments;
// Whether a TOGGLE communciation sequence is active
volatile bool toggling = false;

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

    // surface can now be freed
    SDL_FreeSurface(icon_surface);
}

int write_to_socket(const char* message, WindowControlArguments* args) {
    /*
        Write `message` to the write socket in `args`. This is used
        for communication via socket or named pipe to the client app.

        Arguments:
            message (const char*): message to be sent
            args (WindowControlArguments*): struct of args used for communication
                with the client app

        Returns:
            (int): -1 on failure, 1 on a closed pipe or socket, 0 on other success
    */

#ifdef _WIN32
    DWORD wrote_chars;
    if (!WriteFile(args->write_socket_fd, message, (DWORD)strlen(message), &wrote_chars, NULL)) {
        if (GetLastError() == ERROR_INVALID_HANDLE) {
            LOG_INFO("Client app socket closed for writing");
            return 1;
        }
        LOG_ERROR("WriteFile window event to client app socket failed: errno %d", GetLastError());
        return -1;
    }
#else  // UNIX
    int wrote_chars;
    if ((wrote_chars = write(args->write_socket_fd, message, (int)strlen(message))) < 0) {
        if (errno == EBADF) {
            LOG_INFO("Client app socket closed for writing");
            return 1;
        }
        LOG_ERROR("write window event to client app socket failed: errno %d", errno);
        return -1;
    }
#endif
    return 0;
}

void focus_window(SDL_Window* window_to_focus) {
    /*
        Bring the window `window_to_focus` to the front and transfer input focus
        to it.

        Arguments:
            window_to_focus (SDL_Window*): pointer to the target window
    */

    SDL_RestoreWindow(window_to_focus);
    SDL_RaiseWindow(window_to_focus);

    // On Windows, focusing the window requires more manipulation of the window handle itself
#ifdef _WIN32
    SDL_SysWMinfo window_info;
    SDL_VERSION(&window_info.version);
    if (SDL_GetWindowWMInfo(window_to_focus, &window_info)) {
        HWND window_handle = window_info.info.win.window;
        // Bring window to the very top, then remove "topmost" status
        SetWindowPos(window_handle, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);
        SetWindowPos(window_handle, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(window_handle);
    } else {
        LOG_ERROR("SDL_GetWindowWMInfo failed with error %s", SDL_GetError());
    }
#endif
}

int window_control_event_watcher(void* data, SDL_Event* event) {
    /*
        Event watcher to be used in SDL_AddEventWatch to capture
        and communicate window minimize and restore events to the
        client app via socket.

        Arguments:
            data (void*): struct of SDL Window data and socket fd
            event (SDL_Event*): SDL event to be analyzed

        Return:
            (int): 0 on success, -1 on failure
    */

    WindowControlArguments* args = (WindowControlArguments*)data;

    const char* message = NULL;

    if (event->type == SDL_WINDOWEVENT) {
        if (event->window.event == SDL_WINDOWEVENT_MINIMIZED) {
            message = "client:MINIMIZE\n";
        } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            // If the TOGGLE communication sequence is active, then a
            //     return of focus to the protocol indicates that the protocol
            //     window should minimize, instead of communicating FOCUS.
            if (toggling) {
                toggling = false;
                SDL_MinimizeWindow(args->window);
            } else {
                message = "client:FOCUS\n";
            }
        } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            message = "client:UNFOCUS\n";
        }
    } else if (event->type == SDL_QUIT || event->type == SDL_APP_TERMINATING) {
        message = "client:QUIT\n";
    }

    int write_ret = 0;
    if (message) {
        write_ret = write_to_socket(message, args);
        if (write_ret < 0) {
            return -1;
        } else if (write_ret > 0) {
            // Return 0 for success if `write_to_socket` indicates that the pipe
            //     or socket has closed.
            return 0;
        }
    }

    if (event->type == SDL_QUIT || event->type == SDL_APP_TERMINATING) {
#ifdef _WIN32
        CloseHandle(args->write_socket_fd);
        CloseHandle(args->read_socket_fd);
#else  // UNIX
       // use the same fd for R/W on Unix
        close(args->write_socket_fd);
#endif
    }

    return 0;
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

/*
============================
Public Function Implementations
============================
*/

SDL_Window* init_sdl(int target_output_width, int target_output_height, char* name,
                     char* icon_filename) {
    /*
        Attaches the current thread to the specified current input desktop

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

#if CAN_UPDATE_WINDOW_TITLEBAR_COLOR
    // only implemented on macOS so far
    if (skip_taskbar) {
        hide_native_window_taskbar();
    }
#endif  // CAN_UPDATE_WINDOW_TITLEBAR_COLOR

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
    renderer =
        SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
#endif

    // Set icon
    if (strcmp(icon_filename, "") != 0) {
        set_window_icon_from_png(sdl_window, icon_filename);
    }

#if CAN_UPDATE_WINDOW_TITLEBAR_COLOR
    const FractalRGBColor black = {0, 0, 0};
    set_native_window_color(sdl_window, black);
#endif  // CAN_UPDATE_WINDOW_TITLEBAR_COLOR

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
        // If the resize event is for the current window
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window*)data) {
            // Notify video.c about the active resizing
            set_video_active_resizing(true);
        }
    }
    return 0;
}

int share_client_window_events(void* opaque) {
    /*
        Creates a socket between the client app and the desktop protocol
        to capture and send forced window minimize and restore events

        Return:
            (int): 0 on success, -1 on failure
    */

    char buf[64];
    memset(buf, 0, sizeof(buf));

#ifdef _WIN32
    // In Windows, we will refer to fds as sockets, but they are actually a named pipes
    const char* socket_path = "\\\\.\\pipe\\fractal-client.sock";
    HANDLE socket_fd;
    DWORD read_chars;
#else  // UNIX
    const char* socket_path = "fractal-client.sock";  // max 127 char length
    int socket_fd;
    int read_chars;
    struct sockaddr_un addr;
#endif

    // Connect to the client app named pipe or socket server
#ifdef _WIN32
    socket_fd = CreateFileA(socket_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (socket_fd == INVALID_HANDLE_VALUE) {
        LOG_ERROR("CreateFileA error in share_client_window_events: %d", GetLastError());
    }
#else  // UNIX
    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        LOG_ERROR("socket error in share_client_window_events: %d", errno);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    safe_strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("connect error in share_client_window_events: %d", errno);
        return -1;
    }
#endif

    // Set up an event watcher to communicate specific events to the client app
    WindowControlArguments* watcher_args = malloc(sizeof(WindowControlArguments));
    memset(watcher_args, 0, sizeof(WindowControlArguments));
    watcher_args->window = (SDL_Window*)window;
#ifdef _WIN32
    watcher_args->write_socket_fd =
        CreateFileA(socket_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    watcher_args->read_socket_fd = socket_fd;
#else  // UNIX
    watcher_args->write_socket_fd = socket_fd;
#endif
    SDL_AddEventWatch(window_control_event_watcher, watcher_args);

    // Read messages from the client app and handle accordingly
    bool successful_read = true;
#ifdef _WIN32
    while ((successful_read = ReadFile(socket_fd, buf, sizeof(buf), &read_chars, NULL)) == true &&
           read_chars > 0) {
#else  // UNIX
    while ((read_chars = read(socket_fd, buf, sizeof(buf))) > 0) {
#endif
        char* strtok_saveptr;
        char* source_name = safe_strtok(buf, ":", &strtok_saveptr);
        if (source_name && !strncmp(source_name, "server", 6)) {
            char* window_status = safe_strtok(NULL, ":", &strtok_saveptr);
            if (window_status) {
                if (!strncmp(window_status, "MINIMIZE", 8)) {
                    if ((SDL_GetWindowFlags((SDL_Window*)window) & SDL_WINDOW_MINIMIZED) == 0) {
                        SDL_MinimizeWindow((SDL_Window*)window);
                    }
                } else if (!strncmp(window_status, "FOCUS", 5)) {
                    focus_window((SDL_Window*)window);
                } else if (!strncmp(window_status, "TOGGLE_COMPLETE", 15)) {
                    SDL_Delay(1);  // This enforces that a focus window event, if any, will be
                                   //     processed first
                    // If a TOGGLE communication sequence is in progress and no focus window event
                    //     has been detected, then finish the sequence and focus the window.
                    if (toggling) {
                        toggling = false;
                        focus_window((SDL_Window*)window);
                    }
                } else if (!strncmp(window_status, "TOGGLE", 6)) {
                    // This means that a TOGGLE communication sequence has begun and needs to be
                    //     acknowledged
                    toggling = true;
                    write_to_socket("client:TOGGLE_ACK\n", watcher_args);
                } else if (!strncmp(window_status, "QUIT", 4)) {
                    // When the client app quits, the protocol should quit as well
                    SDL_Event quit_event;
                    quit_event.type = SDL_QUIT;
                    SDL_PushEvent(&quit_event);
                }
            }
        }
        memset(buf, 0, sizeof(buf));
    }

    // Handle unsuccessful or 0 char reads appropriately
    if (read_chars < 0 || !successful_read) {
#ifdef _WIN32
        if (GetLastError() == ERROR_INVALID_HANDLE) {
            LOG_INFO("Client app socket closed for reading");
        } else {
            LOG_ERROR("ReadFile error in share_client_window_events, errno: %d", GetLastError());
            return -1;
        }
#else  // UNIX
        if (errno == EBADF) {
            LOG_INFO("Client app socket closed for reading");
        } else {
            LOG_ERROR("read error in share_client_window_events, errno: %d", errno);
            return -1;
        }
#endif
    } else if (read_chars == 0) {
#ifdef _WIN32
        CloseHandle(socket_fd);
        CloseHandle(watcher_args->write_socket_fd);
#else  // UNIX
        close(socket_fd);
#endif
    }

    return 0;
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
