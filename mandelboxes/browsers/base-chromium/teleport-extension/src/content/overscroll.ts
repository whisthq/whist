const element =
  document.compatMode === "BackCompat"
    ? document.body
    : document.documentElement

let topOverscroll = 0

const trim = (delta: number) => {
  let abs = Math.abs(delta)

  if (abs > 8) abs = 8

  return delta > 0 ? abs : -1 * abs
}

const updateTotalMargin = (delta: number) => {
  const update = topOverscroll + delta
  if (update < -50) {
    topOverscroll = -50
  } else {
    topOverscroll = update
  }
}

const handleYOverscroll = (e: WheelEvent) => {
  const canScroll = element.scrollHeight > element.clientHeight
  const isOverscrollingTop = element.scrollTop === 0 && Math.abs(e.deltaY) > 0

  if (!isOverscrollingTop || !canScroll) {
    topOverscroll = 0
    return
  }

  const body = (
    document.getElementsByTagName("BODY") as HTMLCollectionOf<HTMLElement>
  )[0]

  updateTotalMargin(trim(e.deltaY))

  body.style.marginTop = `${Math.abs(topOverscroll)}px`
}

const initYOverscrollHandler = () => {
  window.addEventListener("wheel", handleYOverscroll)
}

export { initYOverscrollHandler }
