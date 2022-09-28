import { fromEventPattern, merge } from "rxjs"
import { filter, distinctUntilChanged } from "rxjs/operators"

const activatedFromSleep = fromEventPattern(
  (handler: any) => chrome.idle.onStateChanged.addListener(handler),
  (handler: any) => chrome.idle.onStateChanged.removeListener(handler),
  (newState: chrome.idle.IdleState) => newState
).pipe(filter((newState: chrome.idle.IdleState) => newState === "active"))

const networkOnline = fromEventPattern(
  (handler: any) => window.addEventListener("online", handler),
  (handler: any) => window.removeEventListener("online", handler),
  () => true
)

const networkOffline = fromEventPattern(
  (handler: any) => window.addEventListener("offline", handler),
  (handler: any) => window.removeEventListener("offline", handler),
  () => false
)

const networkConnected = merge(
  // Initially emit the starting state in case we've missed the first online/offline event.
  [navigator.onLine],
  networkOnline,
  networkOffline
).pipe(distinctUntilChanged())

export { activatedFromSleep, networkConnected }
