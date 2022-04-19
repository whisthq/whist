const element =
  document.compatMode === "BackCompat"
    ? document.body
    : document.documentElement

let topMargin = 0

const trim = (delta: number) => {
  let abs = Math.abs(delta)

  if (abs > 5) abs = 5

  return delta > 0 ? abs : -1 * abs
}

const updateTopMargin = (delta: number) => {
  const update = topMargin + delta
  if (update < -50) {
    topMargin = -50
  } else {
    topMargin = update
  }
}

const handleYOverscroll = (e: WheelEvent) => {
  const canScroll = element.scrollHeight > element.clientHeight
  const isOverscrollingTop = element.scrollTop === 0 && Math.abs(e.deltaY) > 0
  const body = (
    document.getElementsByTagName("BODY") as HTMLCollectionOf<HTMLElement>
  )[0]

  if (!isOverscrollingTop || !canScroll) {
    topMargin = 0
    body.style.marginTop = "0px"
    return
  }

  updateTopMargin(trim(e.deltaY))

  body.style.marginTop = `${Math.abs(topMargin)}px`
}

const initYOverscrollHandler = () => {
  window.addEventListener("wheel", handleYOverscroll)
}

export { initYOverscrollHandler }
