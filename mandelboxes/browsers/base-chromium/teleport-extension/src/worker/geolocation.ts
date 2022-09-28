import { NativeHostMessage, NativeHostMessageType } from "@app/constants/ipc"

const initLocationHandler = (socket: Socket) => {
  // Listen for geolocation request event and send payload to client extension 
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage) => {
    if (msg.type !== ContentScriptMessageType.GEOLOCATION_REQUEST) return

    socket.emit("geolocation_requested", msg.value)
  })
}

export { initLocationHandler }
