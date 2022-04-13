const initOverscroll = () => {
  window.addEventListener("wheel", (e) => {
    console.log("deltaX", e)
  })
}

export { initOverscroll }
