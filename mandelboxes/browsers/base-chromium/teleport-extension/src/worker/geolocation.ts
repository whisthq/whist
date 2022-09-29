import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { getOpenTabs } from "@app/utils/tabs"

import find from "lodash.find"
import { Socket } from "socket.io-client"

const initLocationHandler = (socket: Socket) => {
  // Listen for geolocation request event and send payload to client extension 
  chrome.runtime.onMessage.addListener(async (msg: ContentScriptMessage, sender: chrome.runtime.MessageSender) => {
    if (msg.type !== ContentScriptMessageType.GEOLOCATION_REQUEST) return

    console.log("received geolocation-requested from tab ", sender.tab)

    var clientTabId = undefined
    if (sender.tab !== undefined) {
      const openTabs = await getOpenTabs()
      const foundTab = find(openTabs, (t) => t.tab.id === sender.tab.id)
      clientTabId = foundTab?.clientTabId
    }

    socket.emit(
      "geolocation-requested", 
      msg.value.params, msg.value.metaTagName, clientTabId
    )
  })
}

export { initLocationHandler }
