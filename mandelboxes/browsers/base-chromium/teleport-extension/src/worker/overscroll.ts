import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import {
  maxXOverscroll,
  maxXUpdate,
  minXUpdate,
} from "@app/constants/overscroll"

import { runInActiveTab } from "@app/utils/chrome"
import { sameSign, trim } from "@app/utils/operators"

let throttled = false
let scrollX = 0
let lastTimestamp = 0

const throttle = (ms: number) => {
  throttled = true
  setTimeout(() => {
    throttled = false
  }, ms)
}

const updateOverscroll = (deltaX: number, reset = false) => {
  if (reset) {
    scrollX = 0
    return
  }

  const _scrollX = scrollX + trim(deltaX, minXUpdate, maxXUpdate)

  if (!sameSign(scrollX, _scrollX) && scrollX !== 0) {
    scrollX = 0
    throttle(1000)
  } else {
    scrollX = _scrollX
  }
}

const drawArrow = () => {
  lastTimestamp = Date.now() / 1000
  runInActiveTab((tabID: number) => {
    chrome.tabs.sendMessage(tabID, <ContentScriptMessage>{
      type: ContentScriptMessageType.DRAW_NAVIGATION_ARROW,
      value: {
        progress: Math.abs(scrollX / maxXOverscroll) * 100,
        direction: scrollX < 0 ? "back" : "forward",
        draw: Math.abs(scrollX) <= maxXOverscroll && scrollX !== 0,
      },
    })
  })
}

const navigate = () => {
  const goBack = scrollX < 0
  runInActiveTab((tabID: number) => {
    goBack ? chrome.tabs.goBack(tabID) : chrome.tabs.goForward(tabID)
  })

  scrollX = 0
  throttle(500)
}

const refreshNavigationArrow = () => {
  if (Date.now() / 1000 - lastTimestamp > 1) {
    throttle(150)
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.GESTURE_DETECTED,
      value: {
        reset: true,
      },
    })
  }
}

const initGestureHandler = () => {
  // Fires every rollingLookbackPeriod seconds to see if the wheel is still moving
  setInterval(refreshNavigationArrow, 1)

  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.GESTURE_DETECTED) return
    if (throttled) return

    // Update the total X overscroll amount
    updateOverscroll(msg.value.offset, msg.value.reset)
    // Draw the appropriate arrow
    drawArrow()
    // Navigate if necessary
    if (Math.abs(scrollX) > maxXOverscroll) navigate()
  })
}

export { initGestureHandler }
