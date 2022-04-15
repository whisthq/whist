import { drawArrow } from "@app/utils/overlays"
import { cyclingArray } from "@app/utils/arrays"

import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 1

let arrow: HTMLDivElement | undefined = undefined
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
