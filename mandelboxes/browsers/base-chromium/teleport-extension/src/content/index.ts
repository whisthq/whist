/*
    The entry point to our content scripts.
    A content script is Javascript code that we run inside the active webpage's DOM.
    For more info, see https://developer.chrome.com/docs/extensions/mv3/content_scripts/.
*/

import { initCursorLockDetection } from "./cursor"
import { initUserAgentSpoofer } from "./userAgent"

// Enable relative mouse mode
initCursorLockDetection()

// Enable Linux user agent spoofing on certain predetermined websites
initUserAgentSpoofer()

var div = document.createElement("div")
div.style.width = "100px"
div.style.height = "100px"
div.style.background = "red"
div.style.color = "white"
div.style.position = "fixed"
div.style.top = "10px"
div.style.left = "10px"
div.innerHTML = "Hello"

document.body.appendChild(div)
