#include <fractal/core/fractal.h>
#include "input_driver.h"

typedef char KeyboardMapping;

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
void update_mapped_keyboard_state(InputDevice* input_device, FractalOSType os_type,
                                  FractalKeyboardState keyboard_state);

/**
 * @brief                          Emit a keyboard press/unpress event
 *
 * @param input_device             The initialized input device to write
 *
 * @param os_type                  The origin OS to use for deciding keymaps
 *
 * @param fractal_keycode              The Fractal keycode to modify
 *
 * @param pressed                  1 for a key press, 0 for a key unpress
 *
 * @returns                        0 on success, -1 on failure
 */
int emit_mapped_key_event(InputDevice* input_device, FractalOSType os_type,
                          FractalKeycode fractal_keycode, int pressed);
