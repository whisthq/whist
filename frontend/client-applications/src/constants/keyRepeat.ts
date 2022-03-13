//
// Initial key repeat values
// These correspond to the amount of time (in milliseconds) that the key is held down before the key repeat starts.
//

// Obtained experimentally by @philippemnoel
export const KEY_REPEAT_DELAY_MIN_LINUX = 1 
export const KEY_REPEAT_DELAY_MAX_LINUX = 2000 // This one is unbounded, but we'll cap it at 2000ms

// Obtained experimentally by @philippemnoel
export const KEY_REPEAT_DELAY_MIN_MAC = 15
export const KEY_REPEAT_DELAY_MAX_MAC = 120

// Initial key repeat ranges
export const KEY_REPEAT_DELAY_RANGE_MAC = KEY_REPEAT_DELAY_MAX_MAC - KEY_REPEAT_DELAY_MIN_MAC
export const KEY_REPEAT_DELAY_RANGE_LINUX = KEY_REPEAT_DELAY_MAX_LINUX - KEY_REPEAT_DELAY_MIN_LINUX

//
// Key repeat values
// These correspond to the amount of time (in milliseconds) between key repeats.
//

// Obtained experimentally by @philippemnoel
export const KEY_REPEAT_RATE_MIN_LINUX = 1
export const KEY_REPEAT_RATE_MAX_LINUX = 255 // Note: xset will fail if this is set to higher than 255

// Obtained experimentally by @philippemnoel
export const KEY_REPEAT_RATE_MIN_MAC = 2
export const KEY_REPEAT_RATE_MAX_MAC = 120

// Key repeat ranges
export const KEY_REPEAT_RATE_RANGE_MAC = KEY_REPEAT_RATE_MAX_MAC - KEY_REPEAT_RATE_MIN_MAC
export const KEY_REPEAT_RATE_RANGE_LINUX = KEY_REPEAT_RATE_MAX_LINUX - KEY_REPEAT_RATE_MIN_LINUX
