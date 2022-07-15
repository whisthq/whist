import find from "lodash.find"
import { io, Socket } from "socket.io-client"

import { createTab, activateTab, removeTab } from "@app/utils/tabs"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"
import { WhistTab } from "@app/constants/tabs"

let openTabs: WhistTab[] = []

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelayMax: 500,
    transports: ["websocket"],
  })

  return socket
}

const initActivateTabListener = (socket: Socket) => {
  socket.on("activate-tab", async (tabs: chrome.tabs.Tab[]) => {
    const tab = tabs[0]
    const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)

    if (foundTab?.tab?.id === undefined) {
      const createdTab = await createTab({
        url: tab.url,
        active: tab.active,
      })

      openTabs.push(<WhistTab>{
        tab: createdTab,
        clientTabId: tab.id,
      })
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

const initUpdateTabIDListener = (socket: Socket) => {
  socket.on("update-tab-id", (clientTabId: number, serverTabId: number) => {
    openTabs = openTabs.map((t) => {
      if (t.tab.id !== serverTabId) return t
      return {
        clientTabId,
        tab: t.tab,
      }
    })
  })
}

const initCloudTabUpdatedListener = (socket: Socket) => {
  chrome.tabs.onUpdated.addListener(
    (
      tabId: number,
      changeInfo: { url?: string; title?: string },
      tab: chrome.tabs.Tab
    ) => {
      if (changeInfo.url === undefined && changeInfo.title === undefined) return

      const foundTab = find(openTabs, (t) => t.tab.id === tabId)
      socket.emit("tab-updated", foundTab?.clientTabId, tab)
    }
  )
}

const initCloudTabCreatedListener = (socket: Socket) => {
  chrome.tabs.onCreated.addListener((tab: chrome.tabs.Tab) => {
    if (tab.openerTabId !== undefined) {
      socket.emit("tab-created", tab)
      openTabs.push(<WhistTab>{
        tab: tab,
        clientTabId: undefined,
      })
    }
  })
}

export {
  initSocketioConnection,
  initActivateTabListener,
  initCloseTabListener,
  initCloudTabUpdatedListener,
  initCloudTabCreatedListener,
  initUpdateTabIDListener,
}
