let previousYOffset = 0
let totalMargin = 0

// const trim = (delta: number) => {
//     let abs = Math.abs(delta)

//     if(abs > 25) abs = 25
//     if(abs < 5) abs = 5

//     return delta > 0 ? abs : -1 * abs
// }

// const updateTotalMargin = (delta: number) => {
//     if(totalMargin + )
// }

const handleYOverscroll = (e: WheelEvent) => {
  if (previousYOffset - e.offsetY !== 0) {
    previousYOffset = e.offsetY
    return
  }

  previousYOffset = e.offsetY

  console.log(e.deltaY)

  //   const body = (
  //     document.getElementsByTagName("BODY") as HTMLCollectionOf<HTMLElement>
  //   )[0]
  //   const overscrollTop = e.deltaX < 0

  //   if(overscrollTop) {
  //       body.style.marginTop = totalMargin +
  //   }
}

const initYOverscrollHandler = () => {
  window.addEventListener("wheel", handleYOverscroll)
}

export { initYOverscrollHandler }
