const damping = 0.8
const maxOffset = 150

const content =
  document.compatMode === "BackCompat"
    ? document.body
    : document.documentElement

// states
let offset = 0
let rendered = 0
let lastDis = 0
let backFlag = false

let timer: any

const resetFlag = () => {
  clearTimeout(timer)
  timer = setTimeout(() => {
    backFlag = false
  }, 30)
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

  // throw away float part
  const next = offset - ((dis * damping) | 0)

  content.style.transform = `translate3d(0, ${-next}px, 0)`

  rendered = next
  offset = (offset * damping) | 0

  requestAnimationFrame(render)
}

render()

// wheel delta normalizing
// copied from smooth-scrollbar/src/utils/get-delta.js
const DELTA_SCALE = {
  STANDARD: 1,
  OTHERS: -3,
}

const DELTA_MODE = [1.0, 28.0, 500.0]

const getDeltaMode = (mode: any) => DELTA_MODE[mode] || DELTA_MODE[0]

const getDelta = (evt: WheelEvent) => {
  const mode = getDeltaMode(evt.deltaMode)

  return {
    x: (evt.deltaX / DELTA_SCALE.STANDARD) * mode,
    y: (evt.deltaY / DELTA_SCALE.STANDARD) * mode,
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
  const { y } = getDelta(evt)

  // Check if overscrolling is possible
  const onTopEdge = isOnTopEdge(y)
  const onBottomEdge = isOnBottomEdge(y)

  if (!onTopEdge && !onBottomEdge) return

  resetFlag()
  evt.preventDefault()

  if (!backFlag && y) {
    const update = offset + (y * (maxOffset - Math.abs(offset))) / maxOffset
    if ((onTopEdge && update > 0) || (onBottomEdge && update < 0)) {
      offset = 0
    } else {
      offset += update
    }
  }
}

// wheel events handler
const initYOverscrollHandler = () => {
  window.addEventListener("wheel", handler, { passive: false })
}

export { initYOverscrollHandler }
