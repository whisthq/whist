#include "../fractal/core/fractal.h"
#include "../fractal/utils/logging.h"
#include "../fractal/utils/sdlscreeninfo.h"
#include "main.h"
#include "sdl_event_handler.h"
#include "sdl_utils.h"

// Keyboard state variables
extern bool alt_pressed;
extern bool ctrl_pressed;
extern bool lgui_pressed;
extern bool rgui_pressed;

// Main state variables
extern bool exiting;

extern volatile SDL_Window* window;

extern volatile int output_width;
extern volatile int output_height;

int handleWindowSizeChanged(SDL_Event *event);
int handleKeyUpDown(SDL_Event *event);
int handleMouseMotion(SDL_Event *event);
int handleMouseWheel(SDL_Event *event);
int handleMouseButtonUpDown(SDL_Event *event);

int tryHandleSDLEvent(void) {
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        if (handleSDLEvent(&event) != 0) {
            return -1;
        }
    }
    return 0;
}

int handleSDLEvent(SDL_Event *event) {
    switch (event->type) {
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                if (handleWindowSizeChanged(event) != 0) {
                    return -1;
                }
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (handleKeyUpDown(event) != 0) {
                return -1;
            }
            break;
        case SDL_MOUSEMOTION:
            if (handleMouseMotion(event) != 0) {
                return -1;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (handleMouseButtonUpDown(event) != 0) {
                return -1;
            }
            break;
        case SDL_MOUSEWHEEL:
            if (handleMouseWheel(event) != 0) {
                return -1;
            }
            break;
        case SDL_QUIT:
            LOG_ERROR("Forcefully Quitting...");
            exiting = true;
            break;
    }
    return 0;
}

int handleWindowSizeChanged(SDL_Event *event) {
    // Let video thread know about the resizing to
    // reinitialize display dimensions
    set_video_active_resizing(false);
    output_width = get_window_pixel_width((SDL_Window *) window);
    output_height = get_window_pixel_height((SDL_Window *) window);
    LOG_INFO(
        "Window %d resized to %dx%d (Physical %dx%d)\n",
        event->window.windowID, event->window.data1,
        event->window.data2, output_width, output_height);

    // Let the server know the new dimensions so that it
    // can change native dimensions for monitor
    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_DIMENSIONS;
    fmsg.dimensions.width = output_width;
    fmsg.dimensions.height = output_height;
    fmsg.dimensions.dpi =
        (int) (96.0 * output_width / get_virtual_screen_width());
    SendFmsg(&fmsg);

    return 0;
}

int handleKeyUpDown(SDL_Event *event) {
    FractalKeycode keycode = (FractalKeycode) event->key.keysym.scancode;
    bool is_pressed = event->key.type == SDL_KEYDOWN;

    // Keep memory of alt/ctrl/lgui/rgui status
    if (keycode == FK_LALT) {
        alt_pressed = is_pressed;
    }
    if (keycode == FK_LCTRL) {
        ctrl_pressed = is_pressed;
    }
    if (keycode == FK_LGUI) {
        lgui_pressed = is_pressed;
    }
    if (keycode == FK_RGUI) {
        rgui_pressed = is_pressed;
    }

    if (ctrl_pressed && alt_pressed && keycode == FK_F4) {
        LOG_INFO("Quitting...");
        exiting = true;
    }

    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_KEYBOARD;
    fmsg.keyboard.code = keycode;
    fmsg.keyboard.pressed = is_pressed;
    fmsg.keyboard.mod = event->key.keysym.mod;
    SendFmsg(&fmsg);

    return 0;
}

// Relative motion is the delta x and delta y from last
// mouse position Absolute mouse position is where it is
// on the screen We multiply by scaling factor so that
// integer division doesn't destroy accuracy
int handleMouseMotion(SDL_Event *event) {
    bool is_relative = SDL_GetRelativeMouseMode() == SDL_TRUE;
    int x, y, height, width;

    if (is_relative) {
        x = event->motion.xrel;
        y = event->motion.yrel;
    } else {
        width = get_window_virtual_width((SDL_Window *) window);
        height = get_window_virtual_height((SDL_Window *) window);
        x = event->motion.x * MOUSE_SCALING_FACTOR / width;
        y = event->motion.y * MOUSE_SCALING_FACTOR / height;
    }

    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_MOUSE_MOTION;
    fmsg.mouseMotion.relative = is_relative;
    fmsg.mouseMotion.x = x;
    fmsg.mouseMotion.y = y;
    SendFmsg(&fmsg);

    return 0;
}

int handleMouseButtonUpDown(SDL_Event *event) {
    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_MOUSE_BUTTON;
    // Record if left / right / middle button
    fmsg.mouseButton.button = event->button.button;
    fmsg.mouseButton.pressed = event->button.type == SDL_MOUSEBUTTONDOWN;
    SendFmsg(&fmsg);

    return 0;
}

int handleMouseWheel(SDL_Event *event) {
    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_MOUSE_WHEEL;
    fmsg.mouseWheel.x = event->wheel.x;
    fmsg.mouseWheel.y = event->wheel.y;
    SendFmsg(&fmsg);

    return 0;
}
