import { initCursorLockDetection } from "./cursor"
import { initUserAgentSpoofer } from "./userAgent"

// Enable relative mouse mode
initCursorLockDetection()

// Enable Linux user agent spoofing on certain predetermined websites
initUserAgentSpoofer()
