#include <fractal/core/fractal.h>

typedef char KeyboardMapping;

int emit_mapped_key_event(InputDevice* input_device, KeyboardMapping* keyboard_mapping, FractalKeycode key_code, int pressed);
