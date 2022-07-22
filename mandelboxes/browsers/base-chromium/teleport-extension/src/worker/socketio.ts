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
      console.log("the open tabs are", openTabs)
      const tab = tabs[0]
      const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)

      if (foundTab?.tab?.id === undefined) {
        createTab(
          {
            url: tab.url,
            active: tab.active,
          },
          (createdTab) => {
            chrome.storage.local.set({
              openTabs: [
                ...openTabs,
                {
                  tab: createdTab,
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

const initUpdateTabIDListener = (socket: Socket) => {
  socket.on("update-tab-id", ([clientTabId, serverTabId]: [number, number]) => {
    chrome.storage.local.get(["openTabs"], ({ openTabs }) => {
      openTabs = openTabs.map((t: WhistTab) => {
        if (t.tab.id !== serverTabId) return t
        return {
          clientTabId,
          tab: t.tab,
        }
      })
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
          openTabs.push(<WhistTab>{
            tab: tab,
            clientTabId: undefined,
          })
          socket.emit("tab-created", tab)
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
  initUpdateTabIDListener,
}
