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
} from "@app/constants/overscroll"

// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 1.5

let arrow: HTMLDivElement | undefined = undefined
let previousYDeltas = cyclingArray<number>(10, [])

let overscroll = {
  previousXOffset: 0,
  rollingDelta: 0,
  lastTimestamp: 0,
  lastArrowDirection: undefined,
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

const updateOverscroll = (e: WheelEvent) => {
  const trimmedDelta = Math.min(Math.abs(e.deltaX), MAXIMUM_X_UPDATE)
  if (e.deltaX < 0) {
    overscroll.rollingDelta - trimmedDelta
  } else {
    overscroll.rollingDelta + trimmedDelta
  }
  overscroll.lastTimestamp = now()
}

const detectGesture = (e: WheelEvent) => {
  // If the user is scrolling within the page (i.e not overscrolling), abort
  if (isScrollingVertically(e) || isScrollingHorizontally(e)) return

  // Trakc how much has been overscrolling horizontally in total
  updateOverscroll(e)

  // If there hasn't been much overscroll, don't do anything
  if (Math.abs(overscroll.rollingDelta) < MINIMUM_X_OVERSCROLL) return

  if (Math.abs(overscroll.rollingDelta) > MAXIMUM_X_OVERSCROLL) {
    arrow?.remove()
    arrow = undefined
  }

  // Send the overscroll amount to the worker
  chrome.runtime.sendMessage(<ContentScriptMessage>{
    type: ContentScriptMessageType.GESTURE_DETECTED,
    value: overscroll,
  })
}

const initNavigationArrow = () => {
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.DRAW_NAVIGATION_ARROW) return

    if (
      arrow === undefined ||
      overscroll.lastArrowDirection !== msg.value.direction
    ) {
      arrow?.remove()
      arrow = undefined

      arrow = drawArrow(document, msg.value.direction) as HTMLDivElement
      overscroll.lastArrowDirection = msg.value.direction
    }

    if (msg.value.direction) arrow.style.opacity = msg.value.opacity

    if (msg.value.direction === "back") {
      arrow.style.left = msg.value.offset
    } else {
      arrow.style.right = msg.value.offset
    }
  })
}

const refreshNavigationArrow = () => {
  if (now() - overscroll.lastTimestamp > rollingLookbackPeriod) {
    arrow?.remove()
    arrow = undefined
  }
}

const initSwipeGestures = () => {
  // Fires whenever the wheel moves
  window.addEventListener("wheel", detectGesture)
  // Fires every rollingLookbackPeriod seconds to see if the wheel is still moving
  setInterval(refreshNavigationArrow, rollingLookbackPeriod)

  initNavigationArrow()
}

export { initSwipeGestures }
