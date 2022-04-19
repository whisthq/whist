const element =
  document.compatMode === "BackCompat"
    ? document.body
    : document.documentElement

let topOverscroll = 0

const trim = (delta: number) => {
  let abs = Math.abs(delta)

  if (abs > 25) abs = 25
  if (abs < 5) abs = 5

  return delta > 0 ? abs : -1 * abs
}

const updateTotalMargin = (delta: number) => {
  if (topOverscroll + delta < 50) {
    topOverscroll += delta
  } else {
    topOverscroll = 50
  }
}

const handleYOverscroll = (e: WheelEvent) => {
  const isOverscrollingTop = element.scrollTop === 0 && e.deltaY < 0

  if (!isOverscrollingTop) {
    topOverscroll = 0
    return
  }

  const body = (
    document.getElementsByTagName("BODY") as HTMLCollectionOf<HTMLElement>
  )[0]

  updateTotalMargin(trim(e.deltaY))

  body.style.marginTop = `${topOverscroll}px`

  console.log(body.style.marginTop)
}

const initYOverscrollHandler = () => {
  const canScroll = element.scrollHeight > element.clientHeight
  if (canScroll) window.addEventListener("wheel", handleYOverscroll)
}

export { initYOverscrollHandler }
