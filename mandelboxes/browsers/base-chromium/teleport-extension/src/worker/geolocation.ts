import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { Socket } from "socket.io-client"

const initLocationHandler = (socket: Socket) => {
  // Listen for geolocation request event and send payload to client extension 
  chrome.runtime.onMessage.addListener((msg: ContentScriptMessage, sender: chrome.runtime.MessageSender) => {
    if (msg.type !== ContentScriptMessageType.GEOLOCATION_REQUEST) return

    console.log("received geolocation-requested from tab ", sender.tab)

    socket.emit(
      "geolocation-requested", 
      msg.value.params, msg.value.metaTagName, sender.tab?.id
    )
  })
}

export { initLocationHandler }
