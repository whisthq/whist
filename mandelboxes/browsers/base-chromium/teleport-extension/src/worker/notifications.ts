import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { Socket } from "socket.io-client"

const initNotificationHandler = (socket: Socket) => {
    // Listen for notification creation messages (from the server)
    // and send them along to the client
    chrome.runtime.onMessage.addListener(async (msg: ContentScriptMessage, sender: chrome.runtime.MessageSender) => {
        console.log("notification handler", msg);
        if (msg.type !== ContentScriptMessageType.SERVER_NOTIFICATION) return
        console.log("received server notification message from chrome.runtime.addListener", msg.value.title, msg.value.opt);
        socket.emit("server-notification", msg.value.title, msg.value.opt)
    })
    // causes the cloud tabs to not render D:
    /*
   window.addEventListener("message", function(event) {
       console.log("notification handler", event);
       if (event.source != window) {
           return;
       }
       if (event.data.type && (event.data.type === ContentScriptMessageType.SERVER_NOTIFICATION)) {
        console.log("received server notification message from addEventListener", event.data.value.title, event.data.value.opt);
        socket.emit("server-notification", event.data.value.title, event.data.value.opt)
       }
   })
   */

    // Listen for client messages about notification clicks, dismisses, etc
    // and supposedly send another contentscriptmessage
}

export { initNotificationHandler }
