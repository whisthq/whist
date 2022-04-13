let previousOffset = 0

const wheelHandler = (e: WheelEvent) => {
  console.log(
    "Delta X",
    e.deltaX,
    "Overflowing",
    e.offsetX - previousOffset === 0 && Math.abs(e.deltaX) > 25
  )
  previousOffset = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", wheelHandler)
}

export { initOverscroll }
