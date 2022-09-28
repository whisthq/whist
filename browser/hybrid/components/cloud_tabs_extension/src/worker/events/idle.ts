import { fromEventPattern } from "rxjs"
import { filter } from "rxjs/operators"

const activatedFromSleep = fromEventPattern(
  (handler: any) => chrome.idle.onStateChanged.addListener(handler),
  (handler: any) => chrome.idle.onStateChanged.removeListener(handler),
  (newState: chrome.idle.IdleState) => newState
).pipe(filter((newState: chrome.idle.IdleState) => newState === "active"))

const networkConnected = fromEventPattern(
  (handler: any) => window.addEventListener("online", handler),
  (handler: any) => window.removeEventListener("online", handler),
  (event: any) => event
).pipe()

export { activatedFromSleep, networkConnected }
