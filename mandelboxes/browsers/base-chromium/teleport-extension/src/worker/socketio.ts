import { io } from "socket.io-client"

const initSocketioConnection = () => {
  console.log("trying to connect")
  const socket = io(`http://127.0.0.1:32261`, {
    reconnectionDelayMax: 10000,
    transports: ["websocket"]
  })

  socket.on("connect_error", (err) => {
    console.log("connection error", err)
  })
}

export { initSocketioConnection }
