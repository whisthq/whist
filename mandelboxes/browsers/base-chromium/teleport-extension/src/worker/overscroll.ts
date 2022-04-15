import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { MAXIMUM_X_OVERSCROLL } from "@app/constants/overscroll"

import { runInActiveTab } from "@app/utils/chrome"

let throttled = false

const navigationArrowOffset = (rollingDelta: number) => {
  const slideDistance = MAXIMUM_X_OVERSCROLL - 300
  if (Math.abs(rollingDelta) > slideDistance) return "0px"

  return `-${70 - (70 * Math.abs(rollingDelta)) / slideDistance}px`
}

const navigationArrowOpacity = (rollingDelta: number) =>
  `${Math.max(Math.abs(rollingDelta) / MAXIMUM_X_OVERSCROLL, 0.3).toString()}`

const initGestureHandler = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.GESTURE_DETECTED || throttled)
      return

    if (Math.abs(msg.value.rollingDelta) > MAXIMUM_X_OVERSCROLL) {
      throttled = true
      runInActiveTab((tabID: number) =>
        msg.value.rollingDelta < 0
          ? chrome.tabs.goBack(tabID)
          : chrome.tabs.goForward(tabID)
      )

      setTimeout(() => {
        throttled = false
      }, 1500)
    }

    runInActiveTab((tabID: number) => {
      chrome.tabs.sendMessage(tabID, <ContentScriptMessage>{
        type: ContentScriptMessageType.DRAW_NAVIGATION_ARROW,
        value: {
          offset: navigationArrowOffset(msg.value.rollingDelta),
          opacity: navigationArrowOpacity(msg.value.rollingDelta),
          direction: msg.value.rollingDelta < 0 ? "back" : "forward",
          draw: Math.abs(msg.value.rollingDelta) <= MAXIMUM_X_OVERSCROLL,
        },
      })
    })

    return true
  })
}

export { initGestureHandler }
