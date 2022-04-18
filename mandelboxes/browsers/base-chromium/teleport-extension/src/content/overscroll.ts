import { drawArrow } from "@app/utils/overlays"
import { cyclingArray } from "@app/utils/operators"
import { trim } from "@app/utils/operators"
import {
  maxXOverscroll,
  maxXUpdate,
  minXUpdate,
} from "@app/constants/overscroll"

let arrow: any = undefined
let overscrollX = 0
let previousYDeltas = cyclingArray<number>(4, [])
let previousXOffset = 0
let previousArrowDirection: string | undefined = undefined

const removeArrow = (animate: boolean) => {
  arrow?.remove(animate)
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
  if (Math.abs(overscrollX) > maxXOverscroll) return

  if (isScrollingVertically(e) || isScrollingHorizontally(e)) {
    // Update overscroll tracker
    removeArrow(true)
    overscrollX = 0
    return
  } else {
    overscrollX += trim(e.deltaX, minXUpdate, maxXUpdate)
  }
  // If not overscrolled, don't do anything
  if (overscrollX === 0) {
    removeArrow(false)
    return
  }

  // If overscrolled a lot, redirect
  if (Math.abs(overscrollX) > maxXOverscroll) {
    removeArrow(false)
    window.removeEventListener("wheel", detectGesture)

    if (overscrollX < 0) {
      history.back()
    } else {
      history.forward()
    }

    overscrollX = 0
    // If overscrolled a little, draw the arrow
  } else {
    const direction = overscrollX < 0 ? "back" : "forward"
    if (arrow === undefined || previousArrowDirection !== direction) {
      removeArrow(false)
      arrow = drawArrow(document, direction)
      previousArrowDirection = direction
    }

    arrow.update(Math.abs((overscrollX * 100) / maxXOverscroll))
  }
}

const initSwipeGestures = () => {
  // Fires whenever the wheel moves
  setTimeout(() => {
    window.addEventListener("wheel", detectGesture)
  }, 500)

  window.addEventListener("popstate", () => console.log("POP STATE"))
  window.addEventListener("pushstate", () => console.log("PUSH STATE"))
}

export { initSwipeGestures }
