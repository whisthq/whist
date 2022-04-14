import { drawArrow } from "@app/utils/overlays"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { cyclingArray } from "@app/utils/arrays"

// If the rolling delta exceeds this amount (in absolute value), display the arrow
const rollingDeltaThreshold = 10
// If the rolling delta exceeds this amount (in absolute value), navigate
const navigationThreshold = 250
// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 2.5

let previousOffset = 0
let arrow: HTMLDivElement | undefined = undefined
let previousYDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])
let previousXDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])

const rollingDelta = (offsetX: number) => {
  if (offsetX - previousOffset !== 0) return 0

  const now = Date.now() / 1000
  return previousXDeltas.get().reduce((reduced, args) => {
    if (now - args.timestamp > rollingLookbackPeriod) return reduced
    return reduced + args.delta
  }, 0)
}

const detectVerticalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const detectGesture = (e: WheelEvent) => {
  // Keep track of the last 10 X and Y wheel deltas
  previousYDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaY })
  previousXDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaX })

  // If the user is scrolling vertically, abort
  if (detectVerticalScroll()) return

  // Calculate how far the wheel has overscrolled horizontally in the last rollingLookbackPeriod seconds
  const _rollingDelta = rollingDelta(e.offsetX)
  const _rollingDeltaAbs = Math.abs(_rollingDelta)
  previousOffset = e.offsetX

  // If the wheel hasn't moved much, abort and remove all arrow drawings
  if (Math.abs(_rollingDelta) < rollingDeltaThreshold) {
    if (arrow !== undefined) {
      arrow.remove()
      arrow = undefined
    }
    return
  }

  // The wheel has moved, detect which direction and draw the appropriate arrow
  const goBack = _rollingDelta < 0

  const amountToShift =
    _rollingDeltaAbs >= navigationThreshold
      ? "0px"
      : `-${(70 * _rollingDeltaAbs) / navigationThreshold}px`

  if (arrow === undefined)
    arrow = drawArrow(document, goBack ? "left" : "right")

  arrow.style.opacity = Math.max(
    _rollingDeltaAbs / navigationThreshold,
    0.2
  ).toString()

  console.log("Shifting", amountToShift)
  console.log(
    "opacity",
    Math.max(_rollingDeltaAbs / navigationThreshold, 0.2).toString()
  )

  if (goBack) arrow.style.left = amountToShift
  if (!goBack) arrow.style.right = amountToShift

  if (_rollingDeltaAbs >= navigationThreshold && goBack) history.back()
  if (_rollingDeltaAbs >= navigationThreshold && !goBack) history.forward()
}

const refreshArrow = () => {
  const now = Date.now() / 1000
  const mostRecentTime = previousXDeltas.get().at(-1)?.timestamp ?? 0

  if (now - mostRecentTime > rollingLookbackPeriod && arrow !== undefined) {
    arrow.remove()
    arrow = undefined
  }
}

window.addEventListener("wheel", detectGesture)
setInterval(refreshArrow, rollingLookbackPeriod)
