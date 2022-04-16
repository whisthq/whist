import { drawArrow } from "@app/utils/overlays"
import { cyclingArray } from "@app/utils/operators"

import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 1

let arrow: any = undefined
let previousYDeltas = cyclingArray<number>(3, [])
let previousXOffset = 0
let previousArrowDirection: string | undefined = undefined
let lastTimestamp = 0

const now = () => Date.now() / 1000

const isScrollingVertically = (e: WheelEvent) => {
  previousYDeltas.add(e.deltaY)
  return previousYDeltas.get().some((delta: number) => Math.abs(delta) > 10)
}

const isScrollingHorizontally = (e: WheelEvent) => {
  const isScrolling = previousXOffset - e.offsetX !== 0
  previousXOffset = e.offsetX
  return isScrolling
}

const detectGesture = (e: WheelEvent) => {
  // If the user is scrolling within the page (i.e not overscrolling), abort
  if (isScrollingVertically(e) || isScrollingHorizontally(e)) {
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.GESTURE_DETECTED,
      value: {
        offset: e.deltaY,
        direction: "y",
      },
    })
    return
  }

  // Send the overscroll amount to the worker
  chrome.runtime.sendMessage(<ContentScriptMessage>{
    type: ContentScriptMessageType.GESTURE_DETECTED,
    value: {
      offset: e.deltaX,
      direction: "x",
    },
  })
}

const initNavigationArrow = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.DRAW_NAVIGATION_ARROW) return

    console.log(msg)

    if (
      arrow === undefined ||
      previousArrowDirection !== msg.value.direction ||
      !msg.value.draw
    ) {
      arrow?.remove()
      arrow = undefined

      if (msg.value.draw) {
        arrow = drawArrow(document, msg.value.direction)
        previousArrowDirection = msg.value.direction
      }
    }

    if (msg.value.draw) lastTimestamp = now()

    if (msg.value.direction) arrow.update(msg.value.progress)
  })
}

const refreshNavigationArrow = () => {
  if (now() - lastTimestamp > rollingLookbackPeriod) {
    arrow?.remove()
    arrow = undefined
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
