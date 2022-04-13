import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

let previousOffset = 0

const wheelHandler = (e: WheelEvent) => {
  if (e.offsetX - previousOffset === 0 && e.deltaX > 50) {
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.HISTORY_GO_FORWARD,
    })
  }

  if (e.offsetX - previousOffset === 0 && e.deltaX < -50) {
    chrome.runtime.sendMessage(<ContentScriptMessage>{
      type: ContentScriptMessageType.HISTORY_GO_BACK,
    })
  }

  previousOffset = e.offsetX
}

const initOverscroll = () => {
  window.addEventListener("wheel", wheelHandler)
}

export { initOverscroll }
