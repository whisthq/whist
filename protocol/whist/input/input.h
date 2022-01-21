#ifndef INPUT_H
#define INPUT_H
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file input.h
 * @brief This file defines general input-processing functions and toggles
 *        between Windows and Linux servers.
============================
Usage
============================

Toggles dynamically between input receiving on Windows or Linux Ubuntu
computers. You can create an input device to receive input (keystrokes, mouse
clicks, etc.) via CreateInputDevice. You can then send input to the Windows OS
via ReplayUserInput, and use UpdateKeyboardState to sync keyboard state between
local and remote computers (say, sync them to both have CapsLock activated).
Lastly, you can input an array of keycodes using `InputKeycodes`.
*/

/*
============================
Includes
============================
*/

#include <whist/core/whist.h>

/*
============================
Public Functions
============================
*/

typedef struct InputDevice InputDevice;

/** @brief type of InputDevice */
typedef enum {
    WHIST_INPUT_DEVICE_XTEST,
    WHIST_INPUT_DEVICE_UINPUT,
    WHIST_INPUT_DEVICE_WIN32,
    WHIST_INPUT_DEVICE_WESTON,
} InputDeviceType;

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
typedef void (*InputDeviceDestroyFn)();

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

/**
 * @brief                          Create an input device struct to receive user
 *                                 actions to be replayed on a server
 *
 * @param kind					   type of input device to create
 *
 * @returns                        Initialized input device struct defining
 *                                 mouse and keyboard states
 */
InputDevice* create_input_device(InputDeviceType kind);

/**
 * @brief                          Destroy and free the memory of an input
 *                                 device struct
 *
 * @param input_device             A pointer to the initialized input device struct to
 *                                 destroy and free the memory of. The pointer is nullified
 *                                 after the call.
 */
void destroy_input_device(InputDevice* input_device);

/**
 * @brief                          Get the active/inactive state of a key
 *                                 modifier (caps lock, num lock, etc.)
 *
 * @param input_device             The initialized input device to query
 *
 * @param whist_keycode          The Whist keycode of the modifier to
 *                                 query (`FK_CAPSLOCK` or `FK_NUMLOCK`)
 *
 * @returns                        1 if the queried modifier is active,
 *                                 0 if inactive, and -1 on error
 */
int get_keyboard_modifier_state(InputDevice* input_device, WhistKeycode whist_keycode);

/**
 * @brief                          Get the pressed/unpressed state of a
 *                                 keyboard key
 *
 * @param input_device             The initialized input device to query
 *
 * @param whist_keycode          The Whist keycode to query
 *
 * @returns                        1 if the queried key is pressed,
 *                                 0 if unpressed, and -1 on error
 */
int get_keyboard_key_state(InputDevice* input_device, WhistKeycode whist_keycode);

/**
 * @brief                          Determine whether to ignore the client key state
 *
 * @param input_device             The initialized input device to query
 *
 * @param whist_keycode          The Whist keycode to query
 *
 * @param active_pinch             Whether the client has an active pinch gesture
 *
 * @returns                        1 if we ignore the client key state,
 *                                 0 if we take the client key state
 */
int ignore_key_state(InputDevice* input_device, WhistKeycode whist_keycode, bool active_pinch);

/**
 * @brief                          Emit a keyboard press/unpress event
 *
 * @param input_device             The initialized input device to write
 *
 * @param whist_keycode              The Whist keycode to modify
 *
 * @param pressed                  1 for a key press, 0 for a key unpress
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_key_event(InputDevice* input_device, WhistKeycode whist_keycode, int pressed);

/**
 * @brief                          Emit a relative/absolute mouse motion event
 *
 * @param input_device             The initialized input device to write
 *
 * @param x                        The normalized x coordinate of the mouse event,
 *                                 on a scale from 0 to `MOUSE_SCALING_FACTOR` in the
 *                                 case of absolute mouse motion, or signed in pixels
 *                                 in the case of relative mouse motion
 *
 * @param y                        The normalized y coordinate of the mouse event,
 *                                 on a scale from 0 to `MOUSE_SCALING_FACTOR` in the
 *                                 case of absolute mouse motion, or signed in pixels
 *                                 in the case of relative mouse motion
 *
 * @param relative                 1 for relative, 0 for absolute
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_mouse_motion_event(InputDevice* input_device, int32_t x, int32_t y, int relative);

/**
 * @brief                          Emit a mouse button press/unpress event
 *
 * @param input_device             The initialized input device to write
 *
 * @param button                   The mouse button (left, right, or center) to press
 *
 * @param pressed                  1 for a button press, 0 for a button unpress
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_mouse_button_event(InputDevice* input_device, WhistMouseButton button, int pressed);

/**
 * @brief                          Emit a low-resolution vertical or horizontal mouse
 *                                 scroll event, for click-wheel mouse inputs.
 *
 * @param input_device             The initialized input device to write
 *
 * @param x                        Horizontal scroll direction/amount (-1,0,+1 always work)
 *
 * @param y                        Vertical scroll direction/amount (-1,0,+1 always work)
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_low_res_mouse_wheel_event(InputDevice* input_device, int32_t x, int32_t y);

/**
 * @brief                          Emit a high-resolution vertical or horizontal mouse
 *                                 scroll event, for trackpads or smooth mouse wheels
 *
 * @param input_device             The initialized input device to write
 *
 * @param x                        Horizontal scroll value as a float
 *
 * @param y                        Vertical scroll value as a float
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_high_res_mouse_wheel_event(InputDevice* input_device, float x, float y);

/**
 * @brief                          Emit a trackpad multigesture event (e.g. pinch)
 *
 * @param input_device             The initialized input device to write
 *
 * @param d_theta                  How much the fingers rotated during this motion
 *
 * @param d_dist                   How much the fingers pinched during this motion
 *
 * @param gesture_type             The gesture type (rotate, pinch open, pinch close)
 *
 * @param active_gesture           Whether this event happened mid-multigesture
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_multigesture_event(InputDevice* input_device, float d_theta, float d_dist,
                            WhistMultigestureType gesture_type, bool active_gesture);

/**
 * @brief                          Initialize the input system. Should be used prior to every new
 *                                 client connection that will send inputs
 *
 * @param os_type                  The OS type to use for keyboard mapping
 */
void reset_input(WhistOSType os_type);

/**
 * @brief                          Replayed a received user action on a server,
 *                                 by sending it to the OS
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param fcmsg                     The Whist message packet, defining one user
 *                                 action event, to replay on the computer
 *
 * @returns                        True if it replayed the event, False
 *                                 otherwise
 */
bool replay_user_input(InputDevice* input_device, WhistClientMessage* fcmsg);

/**
 * @brief                          Updates the keyboard state on the server to
 *                                 match the client's
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param fcmsg                     The Whist message packet, defining one
 *                                 keyboard event, to update the keyboard state
 */
void update_keyboard_state(InputDevice* input_device, WhistClientMessage* fcmsg);

/**
 * @brief                          Updates the keyboard state on the server to
 *                                 match the client's
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param keycodes                 A pointer to an array of keycodes to sequentially
 *                                 input
 *
 * @param count                    The number of keycodes to read from `keycodes`
 *
 * @returns                        The number of keycodes successfully inputted from
 *                                 the array
 */
size_t input_keycodes(InputDevice* input_device, WhistKeycode* keycodes, size_t count);

#endif  // INPUT_H
