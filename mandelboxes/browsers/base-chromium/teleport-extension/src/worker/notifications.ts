import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { Socket } from "socket.io-client"

const initNotificationHandler = (socket: Socket) => {
    // Listen for notification creation messages (from the server)
    // and send them along to the client
    chrome.runtime.onMessage.addListener(async (msg: ContentScriptMessage, sender: chrome.runtime.MessageSender) => {
        if (msg.type !== ContentScriptMessageType.SERVER_NOTIFICATION) return
        console.log("received server notification message", msg.value.title, msg.value.opt);
        socket.emit("server-notification", msg.value.title, msg.value.opt)
    })
    // Listen for client messages about notification clicks, dismisses, etc
    // and supposedly send another contentscriptmessage
}

export { initNotificationHandler }
