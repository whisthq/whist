import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { cyclingArray } from "@app/utils/arrays"
import { injectResourceIntoDOM } from "@app/utils/dom"

let previousOffsetX = 0
let throttled = false
let previousYDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])
let previousXDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])

const velocity = (distance: number, time: number) => distance / time

const detectVerticalOverscroll = (e: WheelEvent) =>
  e.offsetX - previousOffsetX === 0 &&
  Math.abs(previousXDeltas.getAll().slice(-1).pop()?.delta ?? 0) > 100

const detectHorizontalScroll = () =>
  previousYDeltas.getAll().some((args) => Math.abs(args.delta) > 10)

const navigateOnGesture = (e: WheelEvent) => {
  previousYDeltas.add({ timestamp: Date.now(), delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now(), delta: e.deltaX })

  console.log(previousXDeltas)

  const gestureDetected =
    detectVerticalOverscroll(e) && // The user is overscrolling horizontally
    !detectHorizontalScroll() && // The user isn't scrolling vertically
    !throttled // Ensures we don't fire multiple gesture events in a row

  const leftGestureDetected = gestureDetected && e.deltaX < -100
  const rightGestureDetected = gestureDetected && e.deltaX > 100

  if (leftGestureDetected) injectResourceIntoDOM(document, "js/overscroll.js")

  if (leftGestureDetected || rightGestureDetected) {
    throttled = true
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: leftGestureDetected
        ? ContentScriptMessageType.HISTORY_GO_BACK
        : ContentScriptMessageType.HISTORY_GO_FORWARD,
    })

    setTimeout(() => {
      throttled = false
    }, 2000)
  }

  previousOffsetX = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }
