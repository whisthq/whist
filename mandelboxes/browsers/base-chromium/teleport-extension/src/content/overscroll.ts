import { injectResourceIntoDOM } from "@app/utils/dom"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { cyclingArray } from "@app/utils/arrays"

let previousOffset = 0
let throttled = false
let previousYDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])
let previousXDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])

const detectRightRelease = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  return movementX === 0 && args.d3 > 150 && args.v0 < -500 && args.v1 < -500
}

const detectLeftRelease = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  console.log("d3", args.d3, "v0", args.v0, "v1", args.v1)
  return movementX === 0 && args.d3 < -150 && args.v0 > 500 && args.v1 > 500
}

const detectVerticalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const navigateOnGesture = (e: WheelEvent) => {
  const filter =
    !detectVerticalScroll() && // Check that the user isn't scrolling vertically
    !throttled // Ensure we don't fire multiple gesture events in a row

  if (!filter) return

  previousYDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaX })

  const d0 = previousXDeltas.get().at(-1)?.delta ?? 0 // X displacement at time t (most recent)
  const d1 = previousXDeltas.get().at(-2)?.delta ?? 0 // X displacement at time t-1
  const d2 = previousXDeltas.get().at(-3)?.delta ?? 0 // X displacement at time t-2
  const d3 = previousXDeltas.get().at(-4)?.delta ?? 0 // X displament at time t-3
  const t0 = previousXDeltas.get().at(-1)?.timestamp ?? 1 // t, in sec since epoch
  const t1 = previousXDeltas.get().at(-2)?.timestamp ?? 1 // t - 1, in sec since epoch
  const t2 = previousXDeltas.get().at(-3)?.timestamp ?? 1 // t - 2, in sec since epoch
  const v0 = (d1 - d0) / (t1 - t0) // X velocity at time t (most recent)
  const v1 = (d2 - d1) / (t2 - t1) // X velocity at time t - 1

  const leftGestureDetected = detectLeftRelease({
    offsetX: e.offsetX,
    d3,
    v0,
    v1,
  })

  const rightGestureDetected = detectRightRelease({
    offsetX: e.offsetX,
    d3,
    v0,
    v1,
  })

  if (leftGestureDetected)
    injectResourceIntoDOM(document, "js/overscrollLeft.js")
  if (rightGestureDetected)
    injectResourceIntoDOM(document, "js/overscrollRight.js")

  if (leftGestureDetected || rightGestureDetected) {
    console.log("RELEASE DETECTED")
    throttled = true

    // Wait some time so the left/right arrow can display
    // setTimeout(() => {
    //   chrome.runtime.sendMessage(<ContentScriptMessage>{
    //     type: leftGestureDetected
    //       ? ContentScriptMessageType.HISTORY_GO_BACK
    //       : ContentScriptMessageType.HISTORY_GO_FORWARD,
    //   })
    // }, 200)

    // Don't allow multiple gestures to send within the same 2s interval
    setTimeout(() => {
      throttled = false
    }, 2000)
  }

  previousOffset = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }
