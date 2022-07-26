#include <whist/core/whist.h>
#include <sys/mman.h>
extern "C" {
#include "common.h"
#include "native.h"
#include <whist/utils/command_line.h>
}

#include "sdl_struct.hpp"

/**
 * Handle an SDL event.
 *
 * @param frontend   Frontend instance.
 * @param event      If a frontend event needs to be generated, filled
 *                   with the new event.  Ignored if not.
 * @param sdl_event  SDL event to handle.
 * @return  True if a frontend event was generated, otherwise false.
 */
static bool sdl_handle_event(WhistFrontend* frontend, WhistFrontendEvent* event,
                             const SDL_Event* sdl_event) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    memset(event, 0, sizeof(WhistFrontendEvent));

    if (sdl_event->type == context->internal_event_id) {
        const SDL_UserEvent* user_event = &sdl_event->user;
        switch (user_event->code) {
            case SDL_FRONTEND_EVENT_FILE_DRAG: {
                event->type = FRONTEND_EVENT_FILE_DRAG;
                event->file_drag = *(FrontendFileDragEvent*)user_event->data1;
                munlock(user_event->data1, sizeof(FrontendFileDragEvent));
                free(user_event->data1);
                return true;
            }
            case SDL_FRONTEND_EVENT_FULLSCREEN: {
                // Extract original fullscreen and id values, from the void*'s
                bool fullscreen = (intptr_t)user_event->data1;
                int id = (intptr_t)user_event->data2;
                if (context->windows.contains(id)) {
                    SDL_Window* window = context->windows[id]->window;
                    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                } else {
                    LOG_WARNING(
                        "Window fullscreen event ignored because there is no window of id %d", id);
                }
                break;
            }
            case SDL_FRONTEND_EVENT_WINDOW_TITLE_CHANGE: {
                // Get the title pointer, and convert from void* to id
                const char* title = (const char*)user_event->data1;
                int id = (intptr_t)user_event->data2;
                size_t title_len = strlen(title);
                if (context->windows.contains(id)) {
                    SDL_Window* window = context->windows[id]->window;
                    SDL_SetWindowTitle(window, title);
                } else {
                    LOG_WARNING(
                        "Window title change event ignored because there is no window of id %d",
                        id);
                }
                munlock(user_event->data1, title_len);
                free(user_event->data1);
                break;
            }
            case SDL_FRONTEND_EVENT_TITLE_BAR_COLOR_CHANGE: {
                // Get the color pointer, and convert from void* to id
                int id = (intptr_t)user_event->data2;
                const WhistRGBColor* color = (const WhistRGBColor*)user_event->data1;
                if (context->windows.contains(id)) {
                    SDL_Window* window = context->windows[id]->window;
                    sdl_native_set_titlebar_color(window, color);
                } else {
                    LOG_WARNING(
                        "Window titlebar color change event ignored because there is no window of "
                        "id %d",
                        id);
                }
                munlock(user_event->data1, sizeof(WhistRGBColor));
                free(user_event->data1);
                break;
            }
            case SDL_FRONTEND_EVENT_NOTIFICATION_CALLBACK: {
                SDLWindowContext* window_context = context->windows.begin()->second;
                // TODO: do something with a notification.  For now,
                // this just restores the first window if it is minimised.
                uint32_t flags = SDL_GetWindowFlags(window_context->window);
                if (flags & SDL_WINDOW_MINIMIZED) {
                    SDL_RestoreWindow(window_context->window);
                }
                break;
            }
            case SDL_FRONTEND_EVENT_INTERRUPT: {
                event->type = FRONTEND_EVENT_INTERRUPT;
                return true;
            }
            case SDL_FRONTEND_EVENT_STDIN_EVENT: {
                char* key = (char*)user_event->data1;
                char* value = (char*)user_event->data2;

                if (key == NULL && value == NULL) {
                    // The stdin stream was broken; forward this
                    // information to the caller.
                    event->type = FRONTEND_EVENT_STARTUP_PARAMETER;
                    event->startup_parameter = {NULL, NULL, true};
                    return true;
                }

                if (strlen(key) == 0) {
                    // Silently ignore empty keys
                    break;
                }

                if (key && !strcmp(key, "open-url")) {
                    // Free `key` since we don't pass it to the handler.
                    free(key);
                    if (value == NULL) {
                        break;
                    }
                    event->type = FRONTEND_EVENT_OPEN_URL;
                    event->open_url.url = value;
                    return true;
                }

                if (key && !strcmp(key, "quit")) {
                    // Free `key` and `value` since we don't pass them to the handler.
                    free(key);
                    free(value);
                    event->type = FRONTEND_EVENT_QUIT;
                    return true;
                }

                event->type = FRONTEND_EVENT_STARTUP_PARAMETER;
                event->startup_parameter = {key, value, false};
                return true;
            }
            default: {
                // Warn about unhandled user events, because we should
                // not have sent an event we are not going to handle.
                LOG_WARNING("Unknown SDL user event %d arrived.", user_event->code);
            }
        }
        return false;
    }

    switch (sdl_event->type) {
        case SDL_WINDOWEVENT: {
            switch (sdl_event->window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    event->type = FRONTEND_EVENT_RESIZE;
                    event->resize.width = sdl_event->window.data1;
                    event->resize.height = sdl_event->window.data2;
                    frontend->call->get_window_pixel_size(frontend, 0, &context->latest_pixel_width,
                                                          &context->latest_pixel_height);
                    break;
                }
                case SDL_WINDOWEVENT_LEAVE: {
                    event->type = FRONTEND_EVENT_MOUSE_LEAVE;
                    break;
                }
// Note: We investigated adding the following events on Windows as
// well, but it would require significant work for minimal gain. As
// such, we only handle occlusion on macOS.
#if OS_IS(OS_MACOS)
                case SDL_WINDOWEVENT_OCCLUDED:
                case SDL_WINDOWEVENT_UNOCCLUDED:
#else
                case SDL_WINDOWEVENT_MINIMIZED:
                case SDL_WINDOWEVENT_RESTORED:
#endif  // macOS
                {
                    event->type = FRONTEND_EVENT_VISIBILITY;
                    event->visibility.visible =
                        (sdl_event->window.event == SDL_WINDOWEVENT_UNOCCLUDED ||
                         sdl_event->window.event == SDL_WINDOWEVENT_RESTORED);
                    break;
                }
                default: {
                    event->type = FRONTEND_EVENT_UNHANDLED;
                    break;
                }
            }
            break;
        }
        case SDL_AUDIODEVICEADDED:
        case SDL_AUDIODEVICEREMOVED: {
            event->type = FRONTEND_EVENT_AUDIO_UPDATE;
            break;
        }
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            event->type = FRONTEND_EVENT_KEYPRESS;
            event->keypress.code = (WhistKeycode)sdl_event->key.keysym.scancode;
            event->keypress.pressed = (sdl_event->type == SDL_KEYDOWN);
            event->keypress.mod = (WhistKeymod)sdl_event->key.keysym.mod;
            break;
        }
        case SDL_MOUSEMOTION: {
            event->type = FRONTEND_EVENT_MOUSE_MOTION;
            event->mouse_motion.absolute.x = sdl_event->motion.x;
            event->mouse_motion.absolute.y = sdl_event->motion.y;
            event->mouse_motion.relative.x = sdl_event->motion.xrel;
            event->mouse_motion.relative.y = sdl_event->motion.yrel;
            event->mouse_motion.relative_mode = SDL_GetRelativeMouseMode();
            break;
        }
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
            event->type = FRONTEND_EVENT_MOUSE_BUTTON;
            event->mouse_button.button = (WhistMouseButton)sdl_event->button.button;
            event->mouse_button.pressed = (sdl_event->type == SDL_MOUSEBUTTONDOWN);
            if (event->mouse_button.button == MOUSE_L) {
                // Capture the mouse while the left mouse button is pressed.
                // This lets SDL track the mouse position even when the drag
                // extends outside the window.
                SDL_CaptureMouse((SDL_bool)event->mouse_button.pressed);
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            event->type = FRONTEND_EVENT_MOUSE_WHEEL;
            event->mouse_wheel.momentum_phase =
                (WhistMouseWheelMomentumType)sdl_event->wheel.momentum_phase;
            event->mouse_wheel.delta.x = sdl_event->wheel.x;
            event->mouse_wheel.delta.y = sdl_event->wheel.y;
            event->mouse_wheel.precise_delta.x = sdl_event->wheel.preciseX;
            event->mouse_wheel.precise_delta.y = sdl_event->wheel.preciseY;
            break;
        }
        case SDL_MULTIGESTURE: {
            event->type = FRONTEND_EVENT_GESTURE;
            event->gesture.num_fingers = sdl_event->mgesture.numFingers;
            event->gesture.delta.theta = sdl_event->mgesture.dTheta;
            event->gesture.delta.dist = sdl_event->mgesture.dDist;
            event->gesture.center.x = sdl_event->mgesture.x;
            event->gesture.center.y = sdl_event->mgesture.y;
            event->gesture.type = MULTIGESTURE_NONE;
            break;
        }
        case SDL_PINCH: {
            event->type = FRONTEND_EVENT_GESTURE;
            event->gesture.num_fingers = 2;
            event->gesture.delta.theta = 0;
            event->gesture.delta.dist = sdl_event->pinch.scroll_amount;
            event->gesture.center.x = 0;
            event->gesture.center.y = 0;
            event->gesture.type = MULTIGESTURE_NONE;
            if (sdl_event->pinch.magnification < 0) {
                event->gesture.type = MULTIGESTURE_PINCH_CLOSE;
            } else if (sdl_event->pinch.magnification > 0) {
                event->gesture.type = MULTIGESTURE_PINCH_OPEN;
            }
            break;
        }
        case SDL_DROPFILE: {
            event->type = FRONTEND_EVENT_FILE_DROP;
            event->file_drop.filename = strdup(sdl_event->drop.file);
            SDL_free(sdl_event->drop.file);
            // Get the global mouse position of the drop event.
            SDL_CaptureMouse((SDL_bool) true);
            SDL_GetMouseState(&event->file_drop.position.x, &event->file_drop.position.y);
            SDL_CaptureMouse((SDL_bool) false);
            break;
        }
        case SDL_DROPCOMPLETE: {
            event->type = FRONTEND_EVENT_FILE_DROP;
            event->file_drop.end_drop = true;
            break;
        }
        case SDL_QUIT: {
            event->type = FRONTEND_EVENT_QUIT;
            event->quit.quit_application = sdl_event->quit.quit_app;
            break;
        }
        default: {
            // Ignore unhandled SDL events.
            return false;
        }
    }

    return true;
}

bool sdl_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    if (!event) {
        return SDL_PollEvent(NULL) != 0;
    }

    SDL_Event sdl_event;
    if (!SDL_PollEvent(&sdl_event)) {
        return false;
    }

    bool got_event = sdl_handle_event(frontend, event, &sdl_event);
    if (!got_event) {
        // Fill in a dummy event.
        event->type = FRONTEND_EVENT_UNHANDLED;
    }

    return true;
}

bool sdl_wait_event(WhistFrontend* frontend, WhistFrontendEvent* event, int timeout_ms) {
    WhistTimer timer;
    start_timer(&timer);
    int current_timeout_ms = timeout_ms;

    bool got_event;
    while (1) {
        SDL_Event sdl_event;
        int res;
        if (timeout_ms == 0) {
            // Even if timeout is zero, we still want to drain all
            // pending events.
            res = SDL_PollEvent(&sdl_event);
        } else {
            res = SDL_WaitEventTimeout(&sdl_event, current_timeout_ms);
        }
        if (res == 0) {
            return false;
        }
        got_event = sdl_handle_event(frontend, event, &sdl_event);
        if (got_event) {
            break;
        }

        int elapsed_ms = (int)(get_timer(&timer) * MS_IN_SECOND);
        if (elapsed_ms >= timeout_ms) {
            current_timeout_ms = 0;
        } else {
            current_timeout_ms = timeout_ms - elapsed_ms;
        }
    }

    return true;
}

void sdl_interrupt(WhistFrontend* frontend) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;
    SDL_Event event = {
        .user =
            {
                .type = context->internal_event_id,
                .code = SDL_FRONTEND_EVENT_INTERRUPT,
            },
    };
    SDL_PushEvent(&event);
}

void sdl_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state, int* key_count,
                            int* mod_state) {
    SDLFrontendContext* context = (SDLFrontendContext*)frontend->context;

    // We could technically call SDL_PumpEvents here, but it's not needed unless we
    // find that it gives a latency or performance improvement.

    if (key_state != NULL) {
        *key_state = context->key_state;
    }
    if (key_count != NULL) {
        *key_count = context->key_count;
    }
    if (mod_state != NULL) {
        *mod_state = SDL_GetModState();
    }
}
