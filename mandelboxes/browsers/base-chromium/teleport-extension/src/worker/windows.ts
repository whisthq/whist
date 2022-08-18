import { Socket } from "socket.io-client"

const initWindowCreatedListener = (socket: Socket) => {
  chrome.windows.onCreated.addListener((window) => {
    socket.emit("server-window-created", window)
  }, {
    windowTypes: ["normal"],
  })
}

export { initWindowCreatedListener }