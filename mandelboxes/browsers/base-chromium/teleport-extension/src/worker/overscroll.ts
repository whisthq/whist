import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import {
  MAXIMUM_X_OVERSCROLL,
  MINIMUM_X_OVERSCROLL,
  MAXIMUM_X_UPDATE,
  MINIMUM_X_UPDATE,
} from "@app/constants/overscroll"

import { runInActiveTab } from "@app/utils/chrome"

let throttled = false
let overscroll = {
  previousXOffset: 0,
  rollingDelta: 0,
  lastTimestamp: 0,
  lastArrowDirection: undefined,
}

const trimDelta = (delta: number) => {
  let trimmedDelta = Math.min(Math.abs(delta), MAXIMUM_X_UPDATE)
  trimmedDelta = Math.max(trimmedDelta, MINIMUM_X_UPDATE)
  return delta < 0 ? -1 * trimmedDelta : trimmedDelta
}

const updateOverscroll = (e: WheelEvent) => {
  const _updatedRollingDelta = overscroll.rollingDelta + trimDelta(e.deltaX)
  if (
    (overscroll.rollingDelta > 0 && _updatedRollingDelta <= 0) ||
    (overscroll.rollingDelta < 0 && _updatedRollingDelta >= 0)
  ) {
    overscroll.rollingDelta = 0
    throttled = true
    setTimeout(() => {
      throttled = false
    }, 500)
  } else {
    overscroll.rollingDelta = _updatedRollingDelta
  }
  overscroll.lastTimestamp = Date.now() / 1000
}

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

    updateOverscroll(msg.value.e)

    // If there hasn't been much overscroll, don't do anything
    if (Math.abs(overscroll.rollingDelta) < MINIMUM_X_OVERSCROLL) return

    if (Math.abs(overscroll.rollingDelta) > MAXIMUM_X_OVERSCROLL) {
      throttled = true
      runInActiveTab((tabID: number) =>
        overscroll.rollingDelta < 0
          ? chrome.tabs.goBack(tabID)
          : chrome.tabs.goForward(tabID)
      )

      setTimeout(() => {
        throttled = false
      }, 1500)
    }

    runInActiveTab((tabID: number) => {
      overscroll.rollingDelta = 0
      chrome.tabs.sendMessage(tabID, <ContentScriptMessage>{
        type: ContentScriptMessageType.DRAW_NAVIGATION_ARROW,
        value: {
          offset: navigationArrowOffset(overscroll.rollingDelta),
          opacity: navigationArrowOpacity(overscroll.rollingDelta),
          direction: overscroll.rollingDelta < 0 ? "back" : "forward",
          draw: Math.abs(overscroll.rollingDelta) <= MAXIMUM_X_OVERSCROLL,
        },
      })
    })

    return true
  })
}

export { initGestureHandler }
