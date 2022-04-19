let previousYOffset = 0

const handleYOverscroll = (e: any) => {
  console.log("Scroll", e)
}

const initYOverscrollHandler = () => {
  window.addEventListener("scroll", handleYOverscroll)
}

export { initYOverscrollHandler }
