import { io } from "socket.io-client"

const initSocketioConnection = () => {
  console.log("trying to connect")
  const socket = io(`http://localhost:32261`, {
    reconnectionDelayMax: 10000,
  })

  socket.on("connect_error", (err) => {
    console.log("connection error", err)
  })
}

export { initSocketioConnection }
