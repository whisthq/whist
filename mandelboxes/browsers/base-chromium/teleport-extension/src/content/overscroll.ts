import { drawArrow } from "@app/utils/overlays"
import { cyclingArray } from "@app/utils/operators"

import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

let arrow: any = undefined
let previousYDeltas = cyclingArray<number>(4, [])
let previousXOffset = 0
let previousArrowDirection: string | undefined = undefined

const now = () => Date.now() / 1000

const removeArrow = () => {
  arrow?.remove()
  arrow = undefined
}

const isScrollingVertically = (e: WheelEvent) => {
  previousYDeltas.add(e.deltaY)
  return previousYDeltas.get().some((delta: number) => Math.abs(delta) > 0)
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

const initSwipeGestures = () => {
  // Fires whenever the wheel moves
  window.addEventListener("wheel", detectGesture)
  // Respond to draw arrow commands from the worker
  initNavigationArrow()

  window.addEventListener("popstate", () => {
    console.log("State popped")
    removeArrow()
  })

  window.addEventListener("pushstate", () => {
    console.log("State pushed")
    removeArrow()
  })
}

export { initSwipeGestures }
