import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

let previousOffset = 0
let hasNavigated = false

const navigateOnGesture = (e: WheelEvent) => {
  if (hasNavigated) {
    window.removeEventListener("wheel", navigateOnGesture)
    return
  }

  console.log(e.deltaX)

  if (e.offsetX - previousOffset === 0 && e.deltaX > 50) {
    hasNavigated = true
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.HISTORY_GO_FORWARD,
    })
  }

  if (e.offsetX - previousOffset === 0 && e.deltaX < -50) {
    hasNavigated = true
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.HISTORY_GO_BACK,
    })
  }

  previousOffset = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", navigateOnGesture)
}

export { initOverscroll }
