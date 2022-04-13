import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

let previousOffset = 0
let throttled = false

const navigateOnGesture = (e: WheelEvent) => {
  console.log(e.deltaX, e.deltaY)

  if (
    e.offsetX - previousOffset === 0 &&
    e.deltaX > 100 &&
    !throttled &&
    Math.abs(e.deltaY) < 10
  ) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: ContentScriptMessageType.HISTORY_GO_FORWARD,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)

    console.log("SCROLLING")
  }

  if (
    e.offsetX - previousOffset === 0 &&
    e.deltaX < -100 &&
    !throttled &&
    Math.abs(e.deltaY) < 10
  ) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: ContentScriptMessageType.HISTORY_GO_BACK,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)

    console.log("SCROLLING")
  }

  previousOffset = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }
