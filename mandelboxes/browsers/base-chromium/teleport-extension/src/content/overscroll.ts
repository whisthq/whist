const initOverscroll = () => {
  window.addEventListener("wheel", (e) => {
    console.log("deltaX", e.deltaX, "displacement", e.screenX - e.clientX)
  })
}

export { initOverscroll }
