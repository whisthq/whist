import { io } from "socket.io-client"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"
import { Socket } from "socket.io-client"

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelayMax: 10000,
    transports: ["websocket"]
  })

  return socket
}

// TODO: Implement socketio event handler
const initSocketioListener = (socket: Socket) => {
    // CLIENT SIDE:

    // socket.emit("open-url", {
    //  newTab: true,
    //  url: "https://google.com"   
    // })

    // SERVER SIDE (i.e. here):
    // socket.on("open-url", (payload) =>{
    //     openUrl(payload.url, payload.newTab)
    // })
}

export { initSocketioConnection, initSocketioListener }
