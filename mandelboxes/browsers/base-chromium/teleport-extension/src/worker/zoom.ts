import { Socket } from "socket.io-client"

const initZoomListener = (socket: Socket) => {
  socket.on("zoom-tab", (args) => {
    console.log("GOT ZOOM TAB", args)
  })
}

export { initZoomListener }