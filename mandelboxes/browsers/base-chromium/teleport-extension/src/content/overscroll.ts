import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

let previousOffset = 0
let throttled = false

const navigateOnGesture = (e: WheelEvent) => {
  console.log(e.deltaX)

  if (e.offsetX - previousOffset === 0 && e.deltaX > 50 && !throttled) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: ContentScriptMessageType.HISTORY_GO_FORWARD,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)
  }

  if (e.offsetX - previousOffset === 0 && e.deltaX < -50) {
    throttled = true
    // chrome.runtime.sendMessage(<ContentScriptMessage>{
    //   type: ContentScriptMessageType.HISTORY_GO_BACK,
    // })

    setTimeout(() => {
      throttled = false
    }, 2000)
  }

  previousOffset = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }
