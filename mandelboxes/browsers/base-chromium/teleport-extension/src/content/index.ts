/*
    The entry point to our content scripts.
    A content script is Javascript code that we run inside the active webpage's DOM.
    For more info, see https://developer.chrome.com/docs/extensions/mv3/content_scripts/.
*/

import { initCursorLockDetection } from "./cursor"
import { initUserAgentSpoofer } from "./userAgent"
import { initFeatureWarnings } from "./notification"
import { initPinchToZoom } from "./zoom"
import { injectResourceIntoDOM } from "@app/utils/dom"

// Enable relative mouse mode
initCursorLockDetection()

// Enable Linux user agent spoofing on certain predetermined websites
initUserAgentSpoofer()

// If the user tries to go to a website that uses your camera/mic, show them a warning
// that this feature is missing
initFeatureWarnings()

// Enable smooth pinch to zoom
initPinchToZoom()

injectResourceIntoDOM(document, "js/geolocation.js")
