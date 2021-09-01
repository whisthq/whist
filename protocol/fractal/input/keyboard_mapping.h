#include <fractal/core/fractal.h>
#include "input_driver.h"

typedef char KeyboardMapping;

int emit_mapped_key_event(InputDevice* input_device, FractalOSType os_type, FractalKeycode key_code, int pressed);

void update_mapped_keyboard_state(InputDevice* input_device, FractalOSType os_type, FractalKeyboardState keyboard_state);
