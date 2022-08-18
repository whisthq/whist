import { Socket } from "socket.io-client"

const initWindowCreatedListener = (socket: Socket) => {
  chrome.windows.onCreated.addListener(
    (window) => {
      const listener = async (
        _tabId: number,
        changeInfo: { url?: string; title?: string; status?: string },
        tab: chrome.tabs.Tab
      ) => {
        if (window.id !== undefined && tab.windowId === window.id) {
          if (changeInfo.status === "loading" && tab.url !== undefined) {
            socket.emit("server-window-created", { ...window, url: tab.url })
            chrome.windows.remove(window.id)
            chrome.windows.onCreated.removeListener(listener)
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
