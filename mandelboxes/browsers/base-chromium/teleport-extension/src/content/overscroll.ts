const initOverscroll = () => {
  window.addEventListener("wheel", (e) => {
    console.log("deltaX", e.screenX - e.clientX)
  })
}

export { initOverscroll }
