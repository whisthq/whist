import { fromEventPattern, fromEvent } from "rxjs"
import { filter, map, switchMap } from "rxjs/operators"
import { Socket } from "socket.io-client"
import find from "lodash.find"
import isEqual from "lodash.isequal"

import { socketConnected } from "@app/worker/events/socketio"
import { whistState } from "@app/worker/utils/state"

const clientCookieAdded = fromEventPattern(
  (handler: any) => chrome.cookies.onChanged.addListener(handler),
  (handler: any) => chrome.cookies.onChanged.removeListener(handler),
  (details: any) => details
).pipe(
  filter(
    ({ cause, removed }: { cause: string; removed: boolean }) =>
      !removed && cause === "explicit"
  ),
  filter(
    ({ cookie }) =>
      find(whistState.alreadyAddedCookies, (c) => isEqual(c, cookie)) ===
      undefined
  ),
  map(({ cookie }) => cookie)
)

// We want to ignore cookies that were removed by explicit calls to chrome.cookies.remove
// and removed because they were overwritten by a new cookie
const clientCookieRemoved = fromEventPattern(
  (handler: any) => chrome.cookies.onChanged.addListener(handler),
  (handler: any) => chrome.cookies.onChanged.removeListener(handler),
  (details: any) => details
).pipe(
  filter(
    ({ cause, removed }: { cause: string; removed: boolean }) =>
      removed && ["expired", "expired_overwrite", "evicted"].includes(cause)
  ),
  map(({ cookie }) => cookie)
)

const serverCookieAdded = socketConnected.pipe(
  switchMap((s: Socket) => fromEvent(s, "client-add-cookie"))
)

const serverCookiesSynced = socketConnected.pipe(
  switchMap((s: Socket) => fromEvent(s, "cookie-sync-complete"))
)

export {
  clientCookieAdded,
  clientCookieRemoved,
  serverCookieAdded,
  serverCookiesSynced,
}
