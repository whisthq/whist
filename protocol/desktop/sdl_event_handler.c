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

int handleWindowSizeChanged(SDL_Event *in_msg, FractalClientMessage *out_fmsg);
int handleKeyUpDown(SDL_Event *in_msg, FractalClientMessage *out_fmsg);
int handleMouseMotion(SDL_Event *in_msg, FractalClientMessage *out_fmsg);
int handleMouseWheel(SDL_Event *in_msg, FractalClientMessage *out_fmsg);
int handleMouseButtonUpDown(SDL_Event *in_msg, FractalClientMessage *out_fmsg);

int tryHandleSDLEvent(void) {
    SDL_Event in_msg;
    if (SDL_PollEvent(&in_msg)) {
        if (handleSDLEvent(&in_msg) != 0) {
            return -1;
        }
    }
    return 0;
}

int handleSDLEvent(SDL_Event *in_msg) {
    FractalClientMessage out_fmsg = {0};

    switch (in_msg->type) {
        case SDL_WINDOWEVENT:
            if (in_msg->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                if (handleWindowSizeChanged(in_msg, &out_fmsg) != 0) {
                    return -1;
                }
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (handleKeyUpDown(in_msg, &out_fmsg) != 0) {
                return -1;
            }
            break;
        case SDL_MOUSEMOTION:
            if (handleMouseMotion(in_msg, &out_fmsg) != 0) {
                return -1;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (handleMouseButtonUpDown(in_msg, &out_fmsg) != 0) {
                return -1;
            }
            break;
        case SDL_MOUSEWHEEL:
            if (handleMouseWheel(in_msg, &out_fmsg) != 0) {
                return -1;
            }
            break;
        case SDL_QUIT:
            LOG_ERROR("Forcefully Quitting...");
            exiting = true;
            break;
    }
    if (out_fmsg.type != 0) {
        SendFmsg(&out_fmsg);
    }
    return 0;
}

int handleWindowSizeChanged(SDL_Event *in_msg, FractalClientMessage *out_fmsg) {
    // Let video thread know about the resizing to
    // reinitialize display dimensions
    set_video_active_resizing(false);
    output_width =
        get_window_pixel_width((SDL_Window*)window);
    output_height =
        get_window_pixel_height((SDL_Window*)window);

    // Let the server know the new dimensions so that it
    // can change native dimensions for monitor
    out_fmsg->type = MESSAGE_DIMENSIONS;
    out_fmsg->dimensions.width = output_width;
    out_fmsg->dimensions.height = output_height;
    out_fmsg->dimensions.dpi =
        (int)(96.0 * output_width /
              get_virtual_screen_width());

    LOG_INFO(
        "Window %d resized to %dx%d (Physical %dx%d)\n",
        in_msg->window.windowID, in_msg->window.data1,
        in_msg->window.data2, output_width, output_height);

    return 0;
}

int handleKeyUpDown(SDL_Event *in_msg, FractalClientMessage *out_fmsg) {
    // Send a keyboard press for this event
    out_fmsg->type = MESSAGE_KEYBOARD;
    out_fmsg->keyboard.code =
        (FractalKeycode)in_msg->key.keysym.scancode;
    out_fmsg->keyboard.mod = in_msg->key.keysym.mod;
    out_fmsg->keyboard.pressed = in_msg->key.type == SDL_KEYDOWN;

    // Keep memory of alt/ctrl/lgui/rgui status
    if (out_fmsg->keyboard.code == FK_LALT) {
        alt_pressed = out_fmsg->keyboard.pressed;
    }
    if (out_fmsg->keyboard.code == FK_LCTRL) {
        ctrl_pressed = out_fmsg->keyboard.pressed;
    }
    if (out_fmsg->keyboard.code == FK_LGUI) {
        lgui_pressed = out_fmsg->keyboard.pressed;
    }
    if (out_fmsg->keyboard.code == FK_RGUI) {
        rgui_pressed = out_fmsg->keyboard.pressed;
    }

    if (ctrl_pressed && alt_pressed &&
        out_fmsg->keyboard.code == FK_F4) {
        LOG_INFO("Quitting...");
        exiting = true;
    }
    return 0;
}

int handleMouseMotion(SDL_Event *in_msg, FractalClientMessage *out_fmsg) {
    // Relative motion is the delta x and delta y from last
    // mouse position Absolute mouse position is where it is
    // on the screen We multiply by scaling factor so that
    // integer division doesn't destroy accuracy
    out_fmsg->type = MESSAGE_MOUSE_MOTION;
    out_fmsg->mouseMotion.relative = SDL_GetRelativeMouseMode();
    out_fmsg->mouseMotion.x = out_fmsg->mouseMotion.relative
                             ? in_msg->motion.xrel
                             : in_msg->motion.x *
                                   MOUSE_SCALING_FACTOR /
                                   get_window_virtual_width(
                                       (SDL_Window*)window);
    out_fmsg->mouseMotion.y =
        out_fmsg->mouseMotion.relative
            ? in_msg->motion.yrel
            : in_msg->motion.y * MOUSE_SCALING_FACTOR /
                  get_window_virtual_height(
                      (SDL_Window*)window);
    return 0;
}


int handleMouseButtonUpDown(SDL_Event *in_msg, FractalClientMessage *out_fmsg) {
    out_fmsg->type = MESSAGE_MOUSE_BUTTON;
    // Record if left / right / middle button
    out_fmsg->mouseButton.button = in_msg->button.button;
    out_fmsg->mouseButton.pressed =
        in_msg->button.type == SDL_MOUSEBUTTONDOWN;
    return 0;
}

int handleMouseWheel(SDL_Event *in_msg, FractalClientMessage *out_fmsg) {
    // Record mousewheel x/y change
    out_fmsg->type = MESSAGE_MOUSE_WHEEL;
    out_fmsg->mouseWheel.x = in_msg->wheel.x;
    out_fmsg->mouseWheel.y = in_msg->wheel.y;
    return 0;
}
