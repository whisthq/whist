import { Socket } from "socket.io-client"

const initWindowCreatedListener = (socket: Socket) => {
  chrome.windows.onCreated.addListener(
    (window) => {
      const listener = (
        _tabId: number,
        changeInfo: { url?: string; title?: string; status?: string },
        tab: chrome.tabs.Tab
      ) => {
        // TECH DEBT: Because extensions can't run in incognito windows we have
        // no way of detecting what URL they open in an incognito window
        // For now, we just close all incognito windows, the long-term solution is 
        // to remove the incognito window option from the right-click context menu
        if (window.incognito && window.id !== undefined) {
          chrome.windows.remove(window.id)
          return
        }

        if (window.id !== undefined && tab.windowId === window.id) {
          if (changeInfo.status === "loading" && tab.url !== undefined) {
            socket.emit("server-window-created", { ...window, url: tab.url })
            chrome.windows.remove(window.id)
            chrome.tabs.onUpdated.removeListener(listener)
          }
        }
      }

      chrome.tabs.onUpdated.addListener(listener)
    },
    {
      windowTypes: ["normal"],
    }
  )
}

export { initWindowCreatedListener }
