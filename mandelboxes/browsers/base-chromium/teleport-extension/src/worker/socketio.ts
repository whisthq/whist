import find from "lodash.find"
import { io, Socket } from "socket.io-client"

import { createTab, activateTab, removeTab } from "@app/utils/tabs"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"
import { WhistTab } from "@app/constants/tabs"

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelayMax: 500,
    transports: ["websocket"],
  })

  return socket
}

const initActivateTabListener = (socket: Socket) => {
  socket.on("activate-tab", (tabs: chrome.tabs.Tab[]) => {
    chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
      const tab = tabs[0]
      const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)
      if (foundTab?.tab?.id === undefined) {
        createTab(
          {
            url: tab.url,
            active: tab.active,
          },
          (_tab: chrome.tabs.Tab) => {
            chrome.storage.local.set({
              openTabs: [
                ...openTabs,
                {
                  tab: _tab,
                  clientTabId: tab.id,
                },
              ],
            })
          }
        )
      } else {
        activateTab(foundTab.tab.id)
      }
    })
  })
}

const initCloseTabListener = (socket: Socket) => {
  socket.on("close-tab", (tabs: chrome.tabs.Tab[]) => {
    chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
      const tab = tabs[0]
      const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)
      if (foundTab?.tab?.id === undefined) {
        console.warn(`Could not remove tab ${tab.id}`)
      } else {
        removeTab(foundTab?.tab?.id)
        chrome.storage.local.set({
          openTabs: openTabs.filter((t: WhistTab) => t.clientTabId !== tab.id),
        })
      }
    })
  })
}

const initHistoryBackListener = (socket: Socket) => {
  socket.on("go-back", (tabIds: number[]) => {
    chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
      const foundTab = find(openTabs, (t) => t.clientTabId === tabIds[0])
      if (foundTab?.tab?.id === undefined) chrome.tabs.goBack(foundTab.tab.id)
    })
  })
}

const initHistoryForwardListener = (socket: Socket) => {
  socket.on("go-forward", (tabIds: number[]) => {
    chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
      const foundTab = find(openTabs, (t) => t.clientTabId === tabIds[0])
      if (foundTab?.tab?.id === undefined)
        chrome.tabs.goForward(foundTab.tab.id)
    })
  })
}

const initCloudTabUpdatedListener = (socket: Socket) => {
  chrome.tabs.onUpdated.addListener(
    (
      tabId: number,
      changeInfo: { url?: string; title?: string; favIconUrl?: string },
      tab: chrome.tabs.Tab
    ) => {
      if (
        changeInfo.url === undefined &&
        changeInfo.title === undefined &&
        changeInfo.favIconUrl === undefined
      )
        return

      chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
        const foundTab = find(openTabs, (t) => t.tab.id === tabId)
        socket.emit("tab-updated", foundTab?.clientTabId, tab)
      })
    }
  )
}

const initCloudTabCreatedListener = (socket: Socket) => {
  chrome.tabs.onUpdated.addListener(
    (
      tabId: number,
      _changeInfo: { url?: string; title?: string },
      tab: chrome.tabs.Tab
    ) => {
      chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
        const foundTab = find(openTabs, (t) => t.tab.id === tabId)

        if (
          tab.openerTabId !== undefined &&
          foundTab === undefined &&
          tab.url !== undefined
        ) {
          if (tab.status === "complete") {
            socket.emit("tab-created", tab)
            removeTab(tabId)
          } else {
            activateTab(tabId)
          }
        }
      })
    }
  )
}

export {
  initSocketioConnection,
  initActivateTabListener,
  initCloseTabListener,
  initCloudTabUpdatedListener,
  initCloudTabCreatedListener,
  initHistoryBackListener,
  initHistoryForwardListener,
}
