// Creates connection to socket.io server

import { io } from "socket.io-client"
import { WhistServer } from "@whist/shared"

const socket = io(WhistServer.DEV, {
  reconnectionDelayMax: 10000,
})

export { socket }
