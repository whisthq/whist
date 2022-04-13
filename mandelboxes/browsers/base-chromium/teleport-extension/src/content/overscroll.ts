const initOverscroll = () => {
  window.addEventListener("wheel", (e) => {
    console.log("deltaX", e.deltaX, "displacement", e.offsetX, "e", e)
  })
}

export { initOverscroll }
