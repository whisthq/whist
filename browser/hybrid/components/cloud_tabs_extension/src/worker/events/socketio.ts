import { zip, fromEvent, of, merge } from "rxjs"
import {
  map,
  switchMap,
  filter,
  share,
  startWith,
  withLatestFrom,
  count,
  take,
} from "rxjs/operators"

import { mandelboxSuccess } from "@app/worker/events/mandelbox"
import { hostSuccess } from "@app/worker/events/host"
import { io, Socket } from "socket.io-client"
import { MandelboxInfo, HostInfo } from "@app/@types/payload"

const socket = zip(mandelboxSuccess, hostSuccess).pipe(
  map(([mandelbox, host]: [MandelboxInfo, HostInfo]) =>
    io(`http://${mandelbox.mandelboxIP}:${host.mandelboxPorts.port_32261}`, {
      reconnectionDelayMax: 500,
      transports: ["websocket"],
    })
  ),
  share()
)

const socketConnected = socket.pipe(
  switchMap((s: Socket) =>
    fromEvent(s, "connected").pipe(
      filter((clientsConnected: number) => clientsConnected === 2),
      withLatestFrom(of(s))
    )
  ),
  map(([, s]: [any, Socket]) => s)
)

const socketDisconnected = socket.pipe(
  switchMap((s: Socket) =>
    fromEvent(s, "disconnect").pipe(withLatestFrom(of(s)))
  ),
  map(([, s]: [any, Socket]) => s)
)

const socketReconnectFailed = merge(socket, socketConnected).pipe(
  switchMap((s: Socket) =>
    fromEvent(s.io, "reconnect_error").pipe(take(15), count())
  )
)

const socketStatus = merge(
  socketConnected.pipe(map(() => true)),
  socketDisconnected.pipe(map(() => false))
).pipe(startWith(false))

export {
  socket,
  socketConnected,
  socketDisconnected,
  socketStatus,
  socketReconnectFailed,
}
