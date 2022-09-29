import { fromEvent } from "rxjs"
import { switchMap, share } from "rxjs/operators"
import { Socket } from "socket.io-client"

import { socket } from "@app/worker/events/socketio"

const geolocationRequested = socket
  .pipe(switchMap((s: Socket) => fromEvent(s, "geolocation-requested")))
  .pipe(share())

export {
  geolocationRequested,
}