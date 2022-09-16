import find from "lodash.find"
import { io, Socket } from "socket.io-client"

import {
  createTab,
  updateTab,
  removeTab,
  getOpenTabs,
  getTab,
} from "@app/utils/tabs"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"

const initTabState = () => {
  chrome.storage.local.clear()
  chrome.storage.sync.set({ cookiesSynced: false })
}

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelayMax: 500,
    transports: ["websocket"],
  })

  return socket
}

const initActivateTabListener = (socket: Socket) => {
  socket.on(
    "activate-tab",
    async ([tabToActivate, reload, spotlightId]: [chrome.tabs.Tab, boolean, number]) => {
      const openTabs = await getOpenTabs()
      const foundTab = find(openTabs, (t) => t.clientTabId === tabToActivate.id)

      if (foundTab?.tab?.id === undefined) {
        createTab({
          url: tabToActivate.url,
          active: tabToActivate.active,
        })?.then((_tab) => {
          if (tabToActivate.id !== undefined) {
            if (tabToActivate.active) {
              socket.emit("tab-activated", tabToActivate.id, spotlightId)
            }

            chrome.storage.local.set({
              [tabToActivate.id]: _tab,
            })
          }
        })
      } else if (!reload) {
        updateTab(foundTab.tab.id, {
          active: tabToActivate.active,
        }).then(() => {
          socket.emit("tab-activated", tabToActivate.id, spotlightId)
        })
      } else {
        const tab = await getTab(foundTab.tab.id)
        const urlToActivate = tabToActivate.url?.replace("cloud:", "")

        updateTab(foundTab.tab.id, {
          active: tabToActivate.active,
          ...(tab.url !== urlToActivate && {
            url: urlToActivate,
          }),
        }).then(() => {
          socket.emit("tab-activated", tabToActivate.id, spotlightId)
        })
      }
    }
  )
}

const initCloseTabListener = (socket: Socket) => {
  socket.on("close-tab", async (tabs: chrome.tabs.Tab[]) => {
    const openTabs = await getOpenTabs()
    const tab = tabs[0]
    const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)

    if (foundTab?.tab?.id === undefined) {
      console.warn(`Could not remove tab ${tab.id}`)
    } else {
      removeTab(foundTab?.tab?.id)
      if (foundTab.clientTabId !== undefined)
        chrome.storage.local.remove(foundTab?.clientTabId?.toString())
    }
  })
}

const initTabRefreshListener = (socket: Socket) => {
  socket.on("refresh-tab", async (tabs: chrome.tabs.Tab[]) => {
    const openTabs = await getOpenTabs()
    const tab = tabs[0]
    const foundTab = find(openTabs, (t) => t.clientTabId === tab.id)

    if (foundTab?.tab?.id !== undefined) {
      chrome.tabs.reload(foundTab?.tab?.id)
    }
  })
}

const initCloudTabUpdatedListener = (socket: Socket) => {
  chrome.tabs.onUpdated.addListener(
    async (
      tabId: number,
      changeInfo: { url?: string; favIconUrl?: string; title?: string },
      tab: chrome.tabs.Tab
    ) => {
      if (
        changeInfo.url === undefined &&
        changeInfo.favIconUrl === undefined &&
        changeInfo.title === undefined
      )
        return

      const openTabs = await getOpenTabs()
      const foundTab = find(openTabs, (t) => t.tab.id === tabId)
      if (foundTab?.tab?.id !== undefined) {
        chrome.scripting.executeScript(
          {
            target: { tabId: foundTab.tab.id },
            func: () => {
              return window.history.length
            },
          },
          (result: any) => {
            socket.emit(
              "tab-updated",
              foundTab?.clientTabId,
              result[0].result,
              tab
            )
          }
        )
      }
    }
  )
}

const initCloudTabCreatedListener = (socket: Socket) => {
  chrome.tabs.onUpdated.addListener(
    async (
      tabId: number,
      changeInfo: { url?: string; title?: string; status?: string },
      tab: chrome.tabs.Tab
    ) => {
      const openTabs = await getOpenTabs()
      const foundTab = find(openTabs, (t) => t.tab.id === tabId)

      if (tab.openerTabId !== undefined && foundTab === undefined) {
        if (changeInfo.status === "loading" && tab.url !== undefined) {
          socket.emit("tab-created", tab)
          removeTab(tabId)
        } else {
          updateTab(tab.openerTabId, { active: true })
        }
      }
    }
  )
}

export {
  initTabState,
  initSocketioConnection,
  initActivateTabListener,
  initCloseTabListener,
  initCloudTabUpdatedListener,
  initCloudTabCreatedListener,
  initTabRefreshListener,
}
