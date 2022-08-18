import { Socket } from "socket.io-client"

const initWindowCreatedListener = (socket: Socket) => {
  chrome.windows.onCreated.addListener((window) => {
    socket.emit("server-window-created", window)
    chrome.windows.remove(window.id)
  }, {
    windowTypes: ["normal"],
  })
}

export { initWindowCreatedListener }