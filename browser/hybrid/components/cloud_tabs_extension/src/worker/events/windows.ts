import { fromEvent } from "rxjs"
import { switchMap } from "rxjs/operators"
import { Socket } from "socket.io-client"

import { socketConnected } from "@app/worker/events/socketio"

const serverWindowCreated = socketConnected.pipe(
  switchMap((s: Socket) => fromEvent(s, "server-window-created"))
)

export { serverWindowCreated }
