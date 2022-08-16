import { Socket } from "socket.io-client"

const initZoomListener = (socket: Socket) => {
  socket.on("zoom-tab", ([zoomChangeInfo]: [{newZoomFactor: number, tabId: number}]) => {
    console.log("setting", zoomChangeInfo.tabId, zoomChangeInfo.newZoomFactor)
    chrome.tabs.setZoom(zoomChangeInfo.tabId, zoomChangeInfo.newZoomFactor)
  })
}

export { initZoomListener }