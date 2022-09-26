import { fromEvent } from "rxjs"
import { switchMap } from "rxjs/operators"
import { Socket } from "socket.io-client"

import { socket } from "@app/worker/events/socketio"

const downloadStarted = socket.pipe(
  switchMap((s: Socket) => fromEvent(s, "download-started"))
)

export { downloadStarted }
