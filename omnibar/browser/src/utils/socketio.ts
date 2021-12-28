// This is the application's main entry point

import { io } from "socket.io-client"
import { WhistServer } from "@whist/shared"

const socket = io(WhistServer.DEV, {
  reconnectionDelayMax: 10000,
})

export { socket }
