import { Socket } from "socket.io-client"

const initWindowCreatedListener = (socket: Socket) => {
  chrome.windows.onCreated.addListener((window) => {
    socket.emit("server-window-created", window)
    if(window.id !== undefined) chrome.windows.remove(window.id)
  }, {
    windowTypes: ["normal"],
  })
}

export { initWindowCreatedListener }