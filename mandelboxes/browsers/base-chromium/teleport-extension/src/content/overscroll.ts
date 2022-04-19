// A lower damping value causes the spring to bounce back more quickly
const damping = 0.85
// A higher maxOffset means the page can bounce further
const maxOffset = 100
const minDelta = 20

// Number of pixels to bounce
let offset = 0
// Tracks the last offset amount
let rendered = 0
// Tracks the difference in offset
let lastDis = 0
// Used to throttle so we don't send too many bounce animations
let backFlag = false
let timer: any

// Get the body HTML element
const content =
  document.compatMode === "BackCompat"
    ? document.body
    : document.documentElement

// For scaling the delta values [DELTA_PIXEL, DELTA_LINE, DELTA_PAGE]
// https://developer.mozilla.org/en-US/docs/Web/API/WheelEvent/deltaMode
const DELTA_MODE = [1.0, 28.0, 500.0]

const resetFlag = () => {
  clearTimeout(timer)
  timer = setTimeout(() => {
    backFlag = false
  }, 40)
}

const trim = (update: number) => {
  let abs = Math.abs(update)
  if (abs > 250) abs = 250

  return update >= 0 ? abs : -1 * abs
}

const render = () => {
  if (!offset && !rendered) {
    lastDis = 0

    return requestAnimationFrame(render)
  }

  const dis = offset - rendered

  if (lastDis * dis < 0) {
    backFlag = true
  }

  lastDis = dis

  const next = offset - ((dis * damping) | 0)

  content.style.transform = `translate3d(0, ${-next}px, 0)`

  rendered = next
  offset = (offset * damping) | 0

  requestAnimationFrame(render)
}

const getDeltaMode = (mode: number) => DELTA_MODE[mode] || DELTA_MODE[0]

const getDelta = (evt: WheelEvent) => {
  const mode = getDeltaMode(evt.deltaMode)

  console.log("The mode is", mode)

  return {
    x: evt.deltaX * mode,
    y: evt.deltaY * mode,
  }
}

const isOnTopEdge = (delta: number) => {
  const { scrollTop } = content
  return scrollTop === 0 && delta <= 0
}

const isOnBottomEdge = (delta: number) => {
  const { scrollTop, scrollHeight, clientHeight } = content
  const max = scrollHeight - clientHeight

  return scrollTop === max && delta >= 0
}

const handler = (evt: WheelEvent) => {
  // Don't overscroll if the page has no scrollbar
  const canScroll = content.scrollHeight > content.clientHeight

  if (!canScroll) return

  let { y } = getDelta(evt)

  // Check if the user has scrolled to the top/bottom limits of the page
  const onTopEdge = isOnTopEdge(y)
  const onBottomEdge = isOnBottomEdge(y)

  if (!onTopEdge && !onBottomEdge) {
    content.style.transform = "none"
    return
  }

  resetFlag()
  evt.preventDefault()

  // If the user is overscrolling, play the animation
  if (!backFlag && y) {
    if (Math.abs(y) < minDelta && y < 0) y = -1 * minDelta
    if (Math.abs(y) < minDelta && y > 0) y = minDelta

    let update = (y * (maxOffset - Math.abs(offset))) / maxOffset

    const updated = trim(offset + update)
    if ((onTopEdge && updated > 0) || (onBottomEdge && updated < 0)) {
      content.style.transform = "none"
      offset = -1
    } else {
      offset = updated
    }
  }
}

render()

const initElasticOverscroll = () => {
  window.addEventListener("wheel", handler, { passive: false })
}

export { initElasticOverscroll }
