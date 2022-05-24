#include "common.h"

// HELPER
// TODO: needs better name
// returns the ID of the window with given SDLWindowID
int get_window_id_from_sdl_id(SDLFrontendContext* context, Uint32 sdl_id) {
    for (const auto& pair : context->windows) {
        int id = pair.first;
        SDLWindowContext* window_context = pair.second;
        if (window_context->window_id == sdl_id) {
            return id;
        }
    }
    LOG_ERROR("Found no window with SDL window ID %d", sdl_id);
    return 0;
}

bool sdl_poll_event(WhistFrontend* frontend, WhistFrontendEvent* event) {
    SDLFrontendContext* context = frontend->context;
    if (!event) {
        return SDL_PollEvent(NULL) != 0;
    }

    // We cannot use SDL_WaitEventTimeout here, because
    // Linux seems to treat a 1ms timeout as an infinite timeout
    SDL_Event sdl_event;
    if (!SDL_PollEvent(&sdl_event)) {
        return false;
    }

    memset(event, 0, sizeof(WhistFrontendEvent));

    if (sdl_event.type == context->file_drag_event_id) {
        event->type = FRONTEND_EVENT_FILE_DRAG;
        event->file_drag = *(FrontendFileDragEvent*)sdl_event.user.data1;
        free(sdl_event.user.data1);
        return true;
    }
    int frontend_window_id = get_window_id_from_sdl_id(context, sdl_event.window.windowID);

    switch (sdl_event.type) {
        case SDL_WINDOWEVENT: {
            switch (sdl_event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    event->type = FRONTEND_EVENT_RESIZE;
                    event->resize.id = frontend_window_id;
                    event->resize.width = sdl_event.window.data1;
                    event->resize.height = sdl_event.window.data2;
                    break;
                }
                case SDL_WINDOWEVENT_LEAVE: {
                    event->type = FRONTEND_EVENT_MOUSE_LEAVE;
                    break;
                }
                case SDL_WINDOWEVENT_MOVED: {
                    event->type = FRONTEND_EVENT_MOVE;
                    event->move.id = frontend_window_id;
                    event->move.x = sdl_event.window.data1;
                    event->move.y = sdl_event.window.data2;
                    break;
                }
                case SDL_WINDOWEVENT_CLOSE: {
                    event->type = FRONTEND_EVENT_CLOSE;
                    event->close.id = frontend_window_id;
                    break;
                                            }
                case SDL_WINDOWEVENT_MINIMIZED: {
                    event->type = FRONTEND_EVENT_MINIMIZE;
                    event->minimize.id = frontend_window_id;
                    break;
                                                }
                case SDL_WINDOWEVENT_RESTORED: {
                    event->type = FRONTEND_EVENT_RESTORE;
                    event->restore.id = frontend_window_id;
                    break;
                                               }
// Note: We investigated adding the following events on Windows as
// well, but it would require significant work for minimal gain. As
// such, we only handle occlusion on macOS.
#ifdef __APPLE__
                case SDL_WINDOWEVENT_OCCLUDED:
                case SDL_WINDOWEVENT_UNOCCLUDED:
                {
                    event->type = FRONTEND_EVENT_VISIBILITY;
                    event->visibility.id = frontend_window_id;
                    event->visibility.visible =
                        (sdl_event.window.event == SDL_WINDOWEVENT_UNOCCLUDED ||
                         sdl_event.window.event == SDL_WINDOWEVENT_RESTORED);
                    break;
                }
#endif  // defined(__APPLE__)
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
            event->keypress.code = (WhistKeycode)sdl_event.key.keysym.scancode;
            event->keypress.pressed = (sdl_event.type == SDL_KEYDOWN);
            event->keypress.mod = sdl_event.key.keysym.mod;
            break;
        }
        case SDL_MOUSEMOTION: {
            event->type = FRONTEND_EVENT_MOUSE_MOTION;
            // need ID because the x,y are relative to the window
            event->mouse_motion.id = frontend_window_id;
            event->mouse_motion.absolute.x = sdl_event.motion.x;
            event->mouse_motion.absolute.y = sdl_event.motion.y;
            event->mouse_motion.relative.x = sdl_event.motion.xrel;
            event->mouse_motion.relative.y = sdl_event.motion.yrel;
            event->mouse_motion.relative_mode = SDL_GetRelativeMouseMode();
            break;
        }
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
            event->type = FRONTEND_EVENT_MOUSE_BUTTON;
            event->mouse_button.button = sdl_event.button.button;
            event->mouse_button.pressed = (sdl_event.type == SDL_MOUSEBUTTONDOWN);
            if (event->mouse_button.button == MOUSE_L) {
                // Capture the mouse while the left mouse button is pressed.
                // This lets SDL track the mouse position even when the drag
                // extends outside the window.
                SDL_CaptureMouse(event->mouse_button.pressed);
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            event->type = FRONTEND_EVENT_MOUSE_WHEEL;
            event->mouse_wheel.momentum_phase =
                (WhistMouseWheelMomentumType)sdl_event.wheel.momentum_phase;
            event->mouse_wheel.delta.x = sdl_event.wheel.x;
            event->mouse_wheel.delta.y = sdl_event.wheel.y;
            event->mouse_wheel.precise_delta.x = sdl_event.wheel.preciseX;
            event->mouse_wheel.precise_delta.y = sdl_event.wheel.preciseY;
            break;
        }
        case SDL_MULTIGESTURE: {
            event->type = FRONTEND_EVENT_GESTURE;
            event->gesture.num_fingers = sdl_event.mgesture.numFingers;
            event->gesture.delta.theta = sdl_event.mgesture.dTheta;
            event->gesture.delta.dist = sdl_event.mgesture.dDist;
            event->gesture.center.x = sdl_event.mgesture.x;
            event->gesture.center.y = sdl_event.mgesture.y;
            event->gesture.type = MULTIGESTURE_NONE;
            break;
        }
        case SDL_PINCH: {
            event->type = FRONTEND_EVENT_GESTURE;
            event->gesture.num_fingers = 2;
            event->gesture.delta.theta = 0;
            event->gesture.delta.dist = sdl_event.pinch.scroll_amount;
            event->gesture.center.x = 0;
            event->gesture.center.y = 0;
            event->gesture.type = MULTIGESTURE_NONE;
            if (sdl_event.pinch.magnification < 0) {
                event->gesture.type = MULTIGESTURE_PINCH_CLOSE;
            } else if (sdl_event.pinch.magnification > 0) {
                event->gesture.type = MULTIGESTURE_PINCH_OPEN;
            }
            break;
        }
        case SDL_DROPFILE: {
            event->type = FRONTEND_EVENT_FILE_DROP;
            event->file_drop.filename = strdup(sdl_event.drop.file);
            SDL_free(sdl_event.drop.file);
            // Get the global mouse position of the drop event.
            SDL_CaptureMouse(true);
            SDL_GetMouseState(&event->file_drop.position.x, &event->file_drop.position.y);
            SDL_CaptureMouse(false);
            break;
        }
        case SDL_DROPCOMPLETE: {
            event->type = FRONTEND_EVENT_FILE_DROP;
            event->file_drop.end_drop = true;
            break;
        }
        case SDL_QUIT: {
            event->type = FRONTEND_EVENT_QUIT;
            event->quit.quit_application = sdl_event.quit.quit_app;
            break;
        }
        default: {
            event->type = FRONTEND_EVENT_UNHANDLED;
            break;
        }
    }

    return true;
}

void sdl_get_keyboard_state(WhistFrontend* frontend, const uint8_t** key_state, int* key_count,
                            int* mod_state) {
    SDLFrontendContext* context = frontend->context;

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
