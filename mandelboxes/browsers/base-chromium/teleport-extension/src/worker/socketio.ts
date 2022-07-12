import find from "lodash.find"
import { io, Socket } from "socket.io-client"

import { createTab, activateTab, removeTab } from "@app/utils/tabs"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"
import { WhistTab } from "@app/constants/tabs"

let tabs: WhistTab[] = []

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelay: 25,
    reconnectionDelayMax: 25,
    transports: ["websocket"],
  })

  return socket
}

const initCreateTabListener = (socket: Socket) => {
  socket.on("create-tab", async (_tabs: chrome.tabs.Tab[]) => {
    const tab = _tabs[0]
    const _tab = await createTab({
      url: tab.url,
      active: tab.active,
    })

    tabs.push(<WhistTab>{
      tab: _tab,
      clientTabId: tab.id,
    })
  })
}

const initActivateTabListener = (socket: Socket) => {
  socket.on("activate-tab", (_tabs: chrome.tabs.Tab[]) => {
    const tab = _tabs[0]
    const _tab = find(tabs, (t) => t.clientTabId === tab.id)
    if (_tab?.tab?.id === undefined) {
      socket.emit("activate-tab-error")
    } else {
      activateTab(_tab.tab.id)
    }
  })
}

const initCloseTabListener = (socket: Socket) => {
  socket.on("close-tab", (_tabs: chrome.tabs.Tab[]) => {
    const tab = _tabs[0]
    const _tab = find(tabs, (t) => t.clientTabId === tab.id)
    if (_tab?.tab?.id === undefined) {
      console.warn(`Could not remove tab ${tab.id}`)
    } else {
      removeTab(_tab?.tab?.id)
    }
  })
}

export {
  initSocketioConnection,
  initCreateTabListener,
  initActivateTabListener,
  initCloseTabListener,
}
