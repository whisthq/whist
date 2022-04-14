import { injectResourceIntoDOM } from "@app/utils/dom"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { cyclingArray } from "@app/utils/arrays"

let previousOffset = 0
let previousYDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])
let previousXDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])

let deltaSum = 0

const backGestureDetected = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  return movementX === 0 && args.d3 < -100 && args.v0 > 300 && args.v1 > 300
}

const forwardGestureDetected = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  return movementX === 0 && args.d3 > 100 && args.v0 < -300 && args.v1 < -300
}

const detectVerticalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const detectGesture = (e: WheelEvent) => {
  previousYDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaX })

  deltaSum += e.deltaX
  console.log("Total delta", deltaSum, "Delta", e.deltaX)

  if (detectVerticalScroll()) return

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

  // chrome.runtime.sendMessage(<ContentScriptMessage>{
  //   type: ContentScriptMessageType.NAVIGATE_HISTORY,
  //   value: goBack ? "back" : "forward",
  // })
}

const drawNavigationArrow = (msg: ContentScriptMessage) => {
  if (msg.type !== ContentScriptMessageType.DRAW_NAVIGATION_ARROW) return

  if (msg.value === "back")
    injectResourceIntoDOM(document, "js/overscrollLeft.js")
  if (msg.value === "forward")
    injectResourceIntoDOM(document, "js/overscrollRight.js")
}

const initSwipeGestures = () => {
  window.addEventListener("wheel", detectGesture)
  chrome.runtime.onMessage.addListener(drawNavigationArrow)
}

export { initSwipeGestures }
