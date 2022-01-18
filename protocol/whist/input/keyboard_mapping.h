#ifndef WHIST_KEYBOARD_MAPPING_H
#define WHIST_KEYBOARD_MAPPING_H

#include <whist/core/whist.h>
#include "input.h"

/**
 * @brief                          Updates the keyboard state on the server to
 *                                 match the client's
 *
 * @param input_device             The initialized input device struct defining
 *                                 mouse and keyboard states for the user
 *
 * @param os_type                  The origin OS to use for deciding keymaps
 *
 * @param keyboard_state           The keyboard state to sync to
 */
void update_mapped_keyboard_state(InputDevice* input_device, WhistOSType os_type,
                                  WhistKeyboardState keyboard_state);

/**
 * @brief                          Emit a keyboard press/unpress event
 *
 * @param input_device             The initialized input device to write
 *
 * @param os_type                  The origin OS to use for deciding keymaps
 *
 * @param whist_keycode          The Whist keycode to modify
 *
 * @param pressed                  1 for a key press, 0 for a key unpress
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_mapped_key_event(InputDevice* input_device, WhistOSType os_type,
                          WhistKeycode whist_keycode, int pressed);

#endif  // WHIST_KEYBOARD_MAPPING_H
