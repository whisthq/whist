import { fromEvent } from "rxjs"
import { map, filter } from "rxjs/operators"

import { openDesiredUrl, sendDesiredUrls } from "@whist/shared"
import { socket } from "@app/utils/socketio"

// Emitted if there was a connection error with the socket server
const socketError = fromEvent(socket, "error")

// Emitted on a connection event (not necessarily successful)
const socketConnection = fromEvent(socket, "connect")

// Emitted when the socket was successfully connected
const connectedToServer = socketConnection.pipe(
  map(() => socket.connected),
  filter((connected: boolean) => connected)
)

// Emitted when the socket was succesfully disconnected
const disconnectedFromServer = socketConnection.pipe(
  map(() => socket.disconnected),
  filter((disconnected: boolean) => disconnected)
)

// Emitted when a client asks us to open a web app for them
const urlOpenRequest = fromEvent(socket, openDesiredUrl.name)

// Emitted when a client gives us a new set of URLs
const newDesiredUrlsReceived = fromEvent(socket, sendDesiredUrls.name)

export {
  socketError,
  connectedToServer,
  disconnectedFromServer,
  urlOpenRequest,
  newDesiredUrlsReceived,
}
