import { io } from "socket.io-client"

import { SOCKETIO_SERVER_URL } from "@app/constants/urls"

const initSocketioConnection = () => {
  const socket = io(SOCKETIO_SERVER_URL, {
    reconnectionDelayMax: 10000,
    transports: ["websocket"]
  })

  return socket
}

export { initSocketioConnection }
