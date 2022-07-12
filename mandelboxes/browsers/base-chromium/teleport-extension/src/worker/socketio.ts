import find from "lodash.find"
import { io, Socket } from "socket.io-client"

import { createTab, activateTab, removeTab } from "@app/utils/tabs"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"
import { WhistTab } from "@app/constants/tabs"

let openTabs: WhistTab[] = []

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelay: 25,
    reconnectionDelayMax: 25,
    transports: ["websocket"],
  })

  return socket
}

const initCreateTabListener = (socket: Socket) => {
  socket.on("create-tab", async (tabs: chrome.tabs.Tab[]) => {
    const tab = tabs[0]
    const createdTab = await createTab({
      url: tab.url,
      active: tab.active,
    })

    openTabs.push(<WhistTab>{
      tab: createdTab,
      clientTabId: tab.id,
    })
  })
}

const initActivateTabListener = (socket: Socket) => {
  socket.on("activate-tab", (tabs: chrome.tabs.Tab[]) => {
    const tab = tabs[0]
    const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)
    if (foundTab?.tab?.id === undefined) {
      socket.emit("activate-tab-error")
    } else {
      activateTab(foundTab.tab.id)
    }
  })
}

const initCloseTabListener = (socket: Socket) => {
  socket.on("close-tab", (tabs: chrome.tabs.Tab[]) => {
    const tab = tabs[0]
    const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)
    if (foundTab?.tab?.id === undefined) {
      console.warn(`Could not remove tab ${tab.id}`)
    } else {
      removeTab(foundTab?.tab?.id)
    }
  })
}

export {
  initSocketioConnection,
  initCreateTabListener,
  initActivateTabListener,
  initCloseTabListener,
}
