const initOverscroll = () => {
  window.addEventListener("wheel", (e: any) => {
    console.log(
      "deltaX",
      e.deltaX,
      "offset",
      e.offsetX,
      "wheelDelta",
      e.wheelDeltaX,
      "screen",
      e.screenX
    )
  })
}

export { initOverscroll }
