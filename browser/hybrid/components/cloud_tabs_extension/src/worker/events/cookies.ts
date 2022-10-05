import { fromEventPattern, fromEvent } from "rxjs"
import { filter, map, switchMap, share } from "rxjs/operators"
import { Socket } from "socket.io-client"
import find from "lodash.find"
import isEqual from "lodash.isequal"

import { socket } from "@app/worker/events/socketio"
import { whistState } from "@app/worker/utils/state"
import { NodeEventHandler } from "rxjs/internal/observable/fromEvent"

const clientCookieAdded = fromEventPattern<chrome.cookies.CookieChangeInfo>(
  (handler: NodeEventHandler) => chrome.cookies.onChanged.addListener(handler),
  (handler: NodeEventHandler) =>
    chrome.cookies.onChanged.removeListener(handler),
  (details: any) => details
).pipe(
  filter(({ cause, removed }) => !removed && cause === "explicit"),
  filter(
    ({ cookie }) =>
      find(whistState.alreadyAddedCookies, (c) => isEqual(c, cookie)) ===
      undefined
  ),
  map(({ cookie }) => cookie)
)

// We want to ignore cookies that were removed by explicit calls to chrome.cookies.remove
// and removed because they were overwritten by a new cookie
const clientCookieRemoved = fromEventPattern<chrome.cookies.CookieChangeInfo>(
  (handler: NodeEventHandler) => chrome.cookies.onChanged.addListener(handler),
  (handler: NodeEventHandler) =>
    chrome.cookies.onChanged.removeListener(handler),
  (details: any) => details
).pipe(
  filter(
    ({ cause, removed }) =>
      removed && ["expired", "expired_overwrite", "evicted"].includes(cause)
  ),
  map(({ cookie }) => cookie)
)

const serverCookieAdded = socket.pipe(
  switchMap((s: Socket) => fromEvent(s, "client-add-cookie"))
)

const serverCookiesSynced = socket.pipe(
  switchMap((s: Socket) => fromEvent(s, "cookie-sync-complete"))
)

export {
  clientCookieAdded,
  clientCookieRemoved,
  serverCookieAdded,
  serverCookiesSynced,
}
