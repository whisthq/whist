import { drawArrow } from "@app/utils/overlays"
import { cyclingArray } from "@app/utils/arrays"

import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import {
  MAXIMUM_X_OVERSCROLL,
  MAXIMUM_X_UPDATE,
  MINIMUM_X_OVERSCROLL,
  MINIMUM_X_UPDATE,
} from "@app/constants/overscroll"

// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 1.5

let arrow: HTMLDivElement | undefined = undefined
let previousYDeltas = cyclingArray<number>(3, [])

let overscroll = {
  previousXOffset: 0,
  rollingDelta: 0,
  lastTimestamp: 0,
  lastArrowDirection: undefined,
  throttle: false,
}

const now = () => Date.now() / 1000

const isScrollingVertically = (e: WheelEvent) => {
  previousYDeltas.add(e.deltaY)
  return previousYDeltas.get().some((delta: number) => Math.abs(delta) > 10)
}

const isScrollingHorizontally = (e: WheelEvent) => {
  const isScrolling = overscroll.previousXOffset - e.offsetX !== 0
  overscroll.previousXOffset = e.offsetX
  return isScrolling
}

const detectGesture = (e: WheelEvent) => {
  // If the user is scrolling within the page (i.e not overscrolling), abort
  if (isScrollingVertically(e) || isScrollingHorizontally(e)) return

  // Send the overscroll amount to the worker
  chrome.runtime.sendMessage(<ContentScriptMessage>{
    type: ContentScriptMessageType.GESTURE_DETECTED,
    value: e,
  })
}

const initNavigationArrow = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.DRAW_NAVIGATION_ARROW) return

    if (
      arrow === undefined ||
      overscroll.lastArrowDirection !== msg.value.direction ||
      !msg.value.draw
    ) {
      arrow?.remove()
      arrow = undefined
      overscroll.rollingDelta = 0

      if (msg.value.draw) {
        arrow = drawArrow(document, msg.value.direction)
        overscroll.lastArrowDirection = msg.value.direction
      }
    }

    if (msg.value.direction)
      (arrow as HTMLDivElement).style.opacity = msg.value.opacity

    if (msg.value.direction === "back") {
      ;(arrow as HTMLDivElement).style.left = msg.value.offset
    } else {
      ;(arrow as HTMLDivElement).style.right = msg.value.offset
    }
  })
}

const refreshNavigationArrow = () => {
  if (now() - overscroll.lastTimestamp > rollingLookbackPeriod) {
    arrow?.remove()
    arrow = undefined
    overscroll.rollingDelta = 0
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
