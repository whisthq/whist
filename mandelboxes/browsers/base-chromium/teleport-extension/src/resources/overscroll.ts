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

const detectLeftRelease = (args: {
  offsetX: number
  d3: number
  v0: number
  v1: number
}) => {
  const movementX = args.offsetX - previousOffset
  return movementX === 0 && args.d3 > 150 && args.v0 > 500 && args.v1 > 500
}

const detectHorizontalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const navigateOnGesture = (e: WheelEvent) => {
  previousYDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaX })

  const d0 = previousXDeltas.get().at(-1)?.delta ?? 0
  const d1 = previousXDeltas.get().at(-2)?.delta ?? 0
  const d2 = previousXDeltas.get().at(-3)?.delta ?? 0
  const d3 = previousXDeltas.get().at(-4)?.delta ?? 0
  const t0 = previousXDeltas.get().at(-1)?.timestamp ?? 1
  const t1 = previousXDeltas.get().at(-2)?.timestamp ?? 1
  const t2 = previousXDeltas.get().at(-3)?.timestamp ?? 1
  const v0 = (d1 - d0) / (t1 - t0)
  const v1 = (d2 - d1) / (t2 - t1)

  const filter =
    !detectHorizontalScroll() && // The user isn't scrolling vertically
    !throttled // Ensures we don't fire multiple gesture events in a row

  const leftGestureDetected =
    filter &&
    detectLeftRelease({
      offsetX: e.offsetX,
      d3,
      v0,
      v1,
    })

  if (leftGestureDetected) drawLeftArrow(document)

  if (leftGestureDetected) {
    throttled = true

    setTimeout(() => {
      // chrome.runtime.sendMessage(<ContentScriptMessage>{
      //   type: leftGestureDetected
      //     ? ContentScriptMessageType.HISTORY_GO_BACK
      //     : ContentScriptMessageType.HISTORY_GO_FORWARD,
      // })
    }, 1000)

    setTimeout(() => {
      throttled = false
    }, 2000)
  }

  previousOffset = e.offsetX
}

window.addEventListener("wheel", navigateOnGesture)
