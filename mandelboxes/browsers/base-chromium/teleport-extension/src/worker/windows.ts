import { Socket } from "socket.io-client"

const initWindowCreatedListener = (socket: Socket) => {
  chrome.windows.onCreated.addListener(
    (window) => {
      const listener = (
        _tabId: number,
        changeInfo: { url?: string; title?: string; status?: string },
        tab: chrome.tabs.Tab
      ) => {
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

chrome.contextMenus.onClicked.addListener(console.log)

export { initWindowCreatedListener }
