import { fromEvent } from "rxjs"
import { map, filter } from "rxjs/operators"

import { socket } from "@app/utils/socketio"

// Emitted if there was a connection error with the socket server
const socketError = fromEvent(socket, "error")

// Emitted on a connection event (not necessarily successful)
const socketConnection = fromEvent(socket, "connect")

// Emitted when the socket was successfully connected
const socketConnected = socketConnection.pipe(
  map(() => socket.connected),
  filter((connected: boolean) => connected)
)

// Emitted when the socket was succesfully disconnected
const socketDisconnected = socketConnection.pipe(
  map(() => socket.disconnected),
  filter((disconnected: boolean) => disconnected)
)

// Emitted when a client asks us to open a web app for them
// const openNewUrl = fromEvent(socket, )
