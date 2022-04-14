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

const backGestureDetected = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  console.log("d3", args.d3, "v0", args.v0, "args.v1", args.v1)
  return movementX === 0 && args.d3 < -150 && args.v0 > 500 && args.v1 > 500
}

const forwardGestureDetected = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  return movementX === 0 && args.d3 > 150 && args.v0 < -500 && args.v1 < -500
}

const detectVerticalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const navigateOnGesture = (e: WheelEvent) => {
  previousYDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaX })

  if (detectVerticalScroll() || throttled) return

  const d0 = previousXDeltas.get().at(-1)?.delta ?? 0 // X displacement at time t (most recent)
  const d1 = previousXDeltas.get().at(-2)?.delta ?? 0 // X displacement at time t-1
  const d2 = previousXDeltas.get().at(-3)?.delta ?? 0 // X displacement at time t-2
  const d3 = previousXDeltas.get().at(-4)?.delta ?? 0 // X displament at time t-3
  const t0 = previousXDeltas.get().at(-1)?.timestamp ?? 1 // t, in sec since epoch
  const t1 = previousXDeltas.get().at(-2)?.timestamp ?? 1 // t - 1, in sec since epoch
  const t2 = previousXDeltas.get().at(-3)?.timestamp ?? 1 // t - 2, in sec since epoch
  const v0 = (d0 - d1) / (t0 - t1) // X velocity at time t (most recent)
  const v1 = (d1 - d2) / (t1 - t2) // X velocity at time t - 1

  const goBack = backGestureDetected({
    offsetX: e.offsetX,
    d3,
    v0,
    v1,
  })

  const goForward = forwardGestureDetected({
    offsetX: e.offsetX,
    d3,
    v0,
    v1,
  })

  previousOffset = e.offsetX
  if (!(goBack || goForward)) return

  throttled = true

  if (goBack) injectResourceIntoDOM(document, "js/overscrollLeft.js")
  if (goForward) injectResourceIntoDOM(document, "js/overscrollRight.js")

  // Wait some time so the left/right arrow can display
  setTimeout(() => {
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: goBack
        ? ContentScriptMessageType.HISTORY_GO_BACK
        : ContentScriptMessageType.HISTORY_GO_FORWARD,
    })
  }, 100)

  // Don't allow multiple gestures to send within the same 2s interval
  setTimeout(() => {
    throttled = false
  }, 1000)
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }
