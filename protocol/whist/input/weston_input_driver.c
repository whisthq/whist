#include "input.h"
#include "input_internal.h"

#include <linux/input-event-codes.h>

/*
============================
Custom Types
============================
*/


typedef struct {
    InputDevice base;
    void *whiston;
} InputDeviceWeston;

/*                    implemented in westoncapture.c                           */
void whiston_inject_mouse_position(void *whiston, int x, int y);
void whiston_inject_mouse_buttons(void *opaque, int button, bool pressed);


/* =========================================================================== */

static void weston_destroy_input_device(InputDeviceWeston* input_device) {}

static int weston_get_keyboard_modifier_state(InputDeviceWeston* input_device,
                                              WhistKeycode whist_keycode) {
    return 0;
}

static int weston_get_keyboard_key_state(InputDeviceWeston* input_device,
                                         WhistKeycode whist_keycode) {
    if ((int)whist_keycode >= KEYCODE_UPPERBOUND) {
        return 0;
    }

    // TODO
    return 0;
}

static int weston_ignore_key_state(InputDeviceWeston* input_device, WhistKeycode whist_keycode,
                                   bool active_pinch) {
    return 0;
}

static int weston_emit_key_event(InputDeviceWeston* input_device, WhistKeycode whist_keycode,
                                 int pressed) {
    return 0;
}

static int weston_emit_mouse_motion_event(InputDeviceWeston* input_device, int32_t x, int32_t y,
                                          int relative) {
	whiston_inject_mouse_position(input_device->whiston, x, y);
    return 0;
}

static int weston_emit_mouse_button_event(InputDeviceWeston* input_device, WhistMouseButton button,
                                          int pressed) {
	int weston_button = 0;
	switch (button) {
	case MOUSE_L:
		weston_button = BTN_LEFT;
		break;
	case MOUSE_MIDDLE:
		weston_button = BTN_MIDDLE;
		break;
	case MOUSE_R:
		weston_button = BTN_RIGHT;
		break;
	case MOUSE_X1:
	case MOUSE_X2:
	default:
		break;
	}

	whiston_inject_mouse_buttons(input_device->whiston, weston_button, pressed);
    return 0;
}

static int weston_emit_low_res_mouse_wheel_event(InputDeviceWeston* input_device, int32_t x,
                                                 int32_t y) {
    return 0;
}

static int weston_emit_high_res_mouse_wheel_event(InputDeviceWeston* input_device, float x,
                                                  float y) {
    return 0;
}

static int weston_emit_multigesture_event(InputDeviceWeston* input_device, float d_theta,
                                          float d_dist, WhistMultigestureType gesture_type,
                                          bool active_gesture) {
    UNUSED(input_device);
    UNUSED(d_theta);
    UNUSED(d_dist);
    UNUSED(gesture_type);
    UNUSED(active_gesture);

    LOG_WARNING("Multigesture events not implemented for Weston driver! ");
    return -1;
}

InputDevice* weston_create_input_device(void* data) {
    LOG_INFO("creating weston driver");

    InputDeviceWeston* ret = safe_zalloc(sizeof(*ret));
    ret->whiston = data;

    InputDevice* base = &ret->base;
    base->device_type = WHIST_INPUT_DEVICE_WESTON;
    base->get_keyboard_key_state =
        (InputDeviceGetKeyboardModifierStateFn)weston_get_keyboard_key_state;
    base->get_keyboard_modifier_state =
        (InputDeviceGetKeyboardModifierStateFn)weston_get_keyboard_modifier_state;
    base->ignore_key_state = (InputDeviceIgnoreKeyStateFn)weston_ignore_key_state;
    base->emit_key_event = (InputDeviceEmitKeyEventFn)weston_emit_key_event;
    base->emit_mouse_motion_event =
        (InputDeviceEmitMouseMotionEventFn)weston_emit_mouse_motion_event;
    base->emit_mouse_button_event =
        (InputDeviceEmitMouseButtonEventFn)weston_emit_mouse_button_event;
    base->emit_low_res_mouse_wheel_event =
        (InputDeviceEmitLowResMouseWheelEventFn)weston_emit_low_res_mouse_wheel_event;
    base->emit_high_res_mouse_wheel_event =
        (InputDeviceEmitHighResMouseWheelEventFn)weston_emit_high_res_mouse_wheel_event;
    base->emit_multigesture_event =
        (InputDeviceEmitMultiGestureEventFn)weston_emit_multigesture_event;
    base->destroy = (InputDeviceDestroyFn)weston_destroy_input_device;

    return base;
}
