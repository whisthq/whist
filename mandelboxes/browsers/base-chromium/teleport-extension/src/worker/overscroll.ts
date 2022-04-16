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
    throttle(750)
  } else {
    scrollX = _scrollX
  }

  console.log("updated", scrollX)
}

const navigationArrowOffset = (scrollX: number) => {
  const slideDistance = maxXOverscroll - 300
  if (Math.abs(scrollX) > slideDistance)
    return `${(70 * Math.abs(scrollX)) / slideDistance}px`

  return `-${70 - (70 * Math.abs(scrollX)) / slideDistance}px`
}

const navigationArrowBorderWidth = (scrollX: number) =>
  `-${15 - (15 * Math.abs(scrollX)) / maxXOverscroll}px`

const drawArrow = () => {
  runInActiveTab((tabID: number) => {
    chrome.tabs.sendMessage(tabID, <ContentScriptMessage>{
      type: ContentScriptMessageType.DRAW_NAVIGATION_ARROW,
      value: {
        offset: navigationArrowOffset(scrollX),
        borderWidth: navigationArrowBorderWidth(scrollX),
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
  throttle(1500)
}

const initGestureHandler = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.GESTURE_DETECTED) return
    if (throttled) return

    // Update the total X overscroll amount
    updateOverscroll(msg.value.offset, msg.value.direction !== "x")
    // Draw the appropriate arrow
    drawArrow()
    // Navigate if necessary
    if (Math.abs(scrollX) > maxXOverscroll) navigate()
  })
}

export { initGestureHandler }
