/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file input_internal.h
 * @brief Internal functions for OS-dependent input device setup.
 */
#ifndef WHIST_INPUT_INPUT_INTERNAL_H
#define WHIST_INPUT_INPUT_INTERNAL_H

typedef int (*InputDeviceGetKeyboardModifierStateFn)(InputDevice* dev, WhistKeycode keycode);
typedef int (*InputDeviceGetKeyboardKeyStateFn)(InputDevice* input_device, WhistKeycode keycode);
typedef int (*InputDeviceIgnoreKeyStateFn)(InputDevice* input_device, WhistKeycode whist_keycode,
                                           bool active_pinch);
typedef int (*InputDeviceEmitKeyEventFn)(InputDevice* input_device, WhistKeycode whist_keycode,
                                         int pressed);
typedef int (*InputDeviceEmitMouseMotionEventFn)(InputDevice* input_device, int32_t x, int32_t y,
                                                 int relative);
typedef int (*InputDeviceEmitMouseButtonEventFn)(InputDevice* input_device, WhistMouseButton button,
                                                 int pressed);
typedef int (*InputDeviceEmitLowResMouseWheelEventFn)(InputDevice* input_device, int32_t x,
                                                      int32_t y);
typedef int (*InputDeviceEmitHighResMouseWheelEventFn)(InputDevice* input_device, float x, float y);
typedef int (*InputDeviceEmitMultiGestureEventFn)(InputDevice* input_device, float d_theta,
                                                  float d_dist, WhistMultigestureType gesture_type,
                                                  bool active_gesture);
typedef void (*InputDeviceDestroyFn)(void);

/** @brief base InputDevice */
struct InputDevice {
    InputDeviceType device_type;

    InputDeviceGetKeyboardKeyStateFn get_keyboard_key_state;
    InputDeviceGetKeyboardModifierStateFn get_keyboard_modifier_state;
    InputDeviceIgnoreKeyStateFn ignore_key_state;
    InputDeviceEmitKeyEventFn emit_key_event;
    InputDeviceEmitMouseMotionEventFn emit_mouse_motion_event;
    InputDeviceEmitMouseButtonEventFn emit_mouse_button_event;
    InputDeviceEmitLowResMouseWheelEventFn emit_low_res_mouse_wheel_event;
    InputDeviceEmitHighResMouseWheelEventFn emit_high_res_mouse_wheel_event;
    InputDeviceEmitMultiGestureEventFn emit_multigesture_event;
    InputDeviceDestroyFn destroy;
};

// Type-specific create functions for different input devices.
InputDevice* xtest_create_input_device(void* data);
InputDevice* uinput_create_input_device(void* data);
InputDevice* win_create_input_device(void* data);
InputDevice* weston_create_input_device(void* data);

#endif /* WHIST_INPUT_INPUT_INTERNAL_H */
