/*
    The entry point to our content scripts.
    A content script is Javascript code that we run inside the active webpage's DOM.
    For more info, see https://developer.chrome.com/docs/extensions/mv3/content_scripts/.
*/

import { initCursorLockDetection } from "./cursor"
import { initUserAgentSpoofer } from "./userAgent"
import { initPinchToZoom } from "./zoom"
import { initLocationSpoofer } from "./geolocation"
import { initNotificationHandler } from "./notifications"

// Enable relative mouse mode
initCursorLockDetection()

// Enable Linux user agent spoofing on certain predetermined websites
initUserAgentSpoofer()

// Enable smooth pinch to zoom
initPinchToZoom()

// Enable location spoofing
initLocationSpoofer()

// Enable notification interception
initNotificationHandler()
