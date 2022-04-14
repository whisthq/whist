import { drawLeftArrow, drawRightArrow } from "@app/utils/overlays"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { cyclingArray } from "@app/utils/arrays"

let previousOffset = 0
let throttled = false
let previousYDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])
let previousXDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])

const detectVerticalOverscroll = (e: WheelEvent) =>
  e.offsetX - previousOffset === 0 &&
  Math.abs(previousXDeltas.get().slice(-1).pop()?.delta ?? 0) > 100

const detectHorizontalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const navigateOnGesture = (e: WheelEvent) => {
  previousYDeltas.add({ timestamp: Date.now(), delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now(), delta: e.deltaX })

  console.log(previousXDeltas.get())

  const gestureDetected =
    detectVerticalOverscroll(e) && // The user is overscrolling horizontally
    !detectHorizontalScroll() && // The user isn't scrolling vertically
    !throttled // Ensures we don't fire multiple gesture events in a row

  const leftGestureDetected = gestureDetected && e.deltaX < -100
  const rightGestureDetected = gestureDetected && e.deltaX > 100

  if (leftGestureDetected) drawLeftArrow(document)
  if (rightGestureDetected) drawRightArrow(document)
  if (leftGestureDetected || rightGestureDetected) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: leftGestureDetected
    //     ? ContentScriptMessageType.HISTORY_GO_BACK
    //     : ContentScriptMessageType.HISTORY_GO_FORWARD,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)
  }

  previousOffset = e.offsetX
}

window.addEventListener("wheel", navigateOnGesture)
