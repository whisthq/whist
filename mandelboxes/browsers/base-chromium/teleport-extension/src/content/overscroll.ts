import { drawArrow } from "@app/utils/overlays"
import { cyclingArray } from "@app/utils/operators"

import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 1

let arrow: any = undefined
let previousYDeltas = cyclingArray<number>(4, [])
let previousXOffset = 0
let previousYOffset = 0
let previousArrowDirection: string | undefined = undefined
let lastTimestamp = 0

const now = () => Date.now() / 1000

const removeArrow = () => {
  arrow?.remove()
  arrow = undefined
}

const isScrollingVertically = (e: WheelEvent) => {
  const isScrolling = previousYOffset - e.offsetY !== 0
  previousYOffset = e.offsetY
  return isScrolling
}

const isScrollingHorizontally = (e: WheelEvent) => {
  const isScrolling = previousXOffset - e.offsetX !== 0
  previousXOffset = e.offsetX
  return isScrolling
}

const detectGesture = (e: WheelEvent) => {
  // Send the overscroll amount to the worker
  chrome.runtime.sendMessage(<ContentScriptMessage>{
    type: ContentScriptMessageType.GESTURE_DETECTED,
    value: {
      offset: e.deltaX,
      reset: isScrollingVertically(e) || isScrollingHorizontally(e),
    },
  })
}

const initNavigationArrow = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.DRAW_NAVIGATION_ARROW) return

    lastTimestamp = now()

    if (!msg.value.draw) {
      removeArrow()
      return
    }

    if (arrow === undefined || previousArrowDirection !== msg.value.direction) {
      removeArrow()
      arrow = drawArrow(document, msg.value.direction)
      previousArrowDirection = msg.value.direction
    }

    arrow.update(msg.value.progress)
  })
}

const refreshNavigationArrow = () => {
  if (now() - lastTimestamp > rollingLookbackPeriod) {
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.GESTURE_DETECTED,
      value: {
        reset: true,
      },
    })
  }
}

const initSwipeGestures = () => {
  // Fires whenever the wheel moves
  window.addEventListener("wheel", detectGesture)
  // Fires every rollingLookbackPeriod seconds to see if the wheel is still moving
  setInterval(refreshNavigationArrow, rollingLookbackPeriod)
  // Respond to draw arrow commands from the worker
  initNavigationArrow()
}

export { initSwipeGestures }
