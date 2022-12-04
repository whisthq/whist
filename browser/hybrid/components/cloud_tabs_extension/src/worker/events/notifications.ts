import { fromEvent, of, zip, map, filter, fromEventPattern } from "rxjs"
import { switchMap, share } from "rxjs/operators"
import { Socket } from "socket.io-client"

import { socket } from "@app/worker/events/socketio"

// defines the notification received event
const notificationReceived = socket.pipe(switchMap((s: Socket) => fromEvent(s, "server-notification"))).pipe(share())

export { notificationReceived }
