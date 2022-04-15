import { drawArrow } from "@app/utils/overlays"
import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { cyclingArray } from "@app/utils/arrays"

// If the rolling delta exceeds this amount (in absolute value), display the arrow
const rollingDeltaThreshold = 10
// If the rolling delta exceeds this amount (in absolute value), navigate
const navigationThreshold = 400
// How many seconds to look back when detecting gestures
const rollingLookbackPeriod = 1.5

let previousOffset = 0
let rollingDelta = 0
let arrow: HTMLDivElement | undefined = undefined
let previousYDeltas = cyclingArray<{ timestamp: number; delta: number }>(10, [])
let mostRecentX = 0
let goBack = false

const detectVerticalScroll = () =>
  previousYDeltas.get().some((args) => Math.abs(args.delta) > 10)

const removeArrow = () => {
  if (arrow !== undefined) {
    arrow.remove()
    arrow = undefined
  }
}

const detectGesture = (e: WheelEvent) => {
  // Keep track of the last 10 X and Y wheel deltas
  previousYDeltas.add({ timestamp: Date.now() / 1000, delta: e.deltaY })

  // If the user is scrolling vertically, abort
  if (detectVerticalScroll() || previousOffset - e.offsetX !== 0) {
    previousOffset = e.offsetX
    return
  }

  // Calculate how far the wheel has overscrolled horizontally in the last rollingLookbackPeriod seconds
  mostRecentX = Date.now() / 1000
  rollingDelta += e.deltaX

  // If the wheel hasn't moved much, abort and remove all arrow drawings
  if (Math.abs(rollingDelta) < rollingDeltaThreshold) {
    rollingDelta = 0
    removeArrow()
    return
  }

  // The wheel has moved, detect which direction and draw the appropriate arrow
  if (rollingDelta < 0 !== goBack) removeArrow()
  goBack = rollingDelta < 0

  const amountToShift =
    Math.abs(rollingDelta) >= navigationThreshold
      ? "0px"
      : `-${70 - (70 * Math.abs(rollingDelta)) / navigationThreshold}px`

  if (arrow === undefined)
    arrow = drawArrow(document, goBack ? "left" : "right")

  arrow.style.opacity = Math.max(
    Math.abs(rollingDelta) / navigationThreshold,
    0.2
  ).toString()

  if (goBack) arrow.style.left = amountToShift
  if (!goBack) arrow.style.right = amountToShift

  if (Math.abs(rollingDelta) < navigationThreshold) return

  chrome.runtime.sendMessage(<ContentScriptMessage>{
    type: ContentScriptMessageType.GESTURE_DETECTED,
    value: goBack ? "back" : "forward",
  })

  removeArrow()
  rollingDelta = 0
}

const refreshArrow = () => {
  const now = Date.now() / 1000

  if (now - mostRecentX > rollingLookbackPeriod) {
    rollingDelta = 0
    removeArrow()
  }
}

const initSwipeGestures = () => {
  setTimeout(() => {
    // Fires whenever the wheel moves
    window.addEventListener("wheel", detectGesture)
    // Fires every rollingLookbackPeriod seconds to see if the wheel is still moving
    setInterval(refreshArrow, rollingLookbackPeriod)
  }, 1000)
}

export { initSwipeGestures }
