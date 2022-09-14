import { fromEventPattern } from "rxjs"
import { filter } from "rxjs/operators"

const activatedFromSleep = fromEventPattern(
  (handler: any) => chrome.idle.onStateChanged.addListener(handler),
  (handler: any) => chrome.idle.onStateChanged.removeListener(handler),
  (newState: chrome.idle.IdleState) => newState
).pipe(filter((newState: chrome.idle.IdleState) => newState === "active"))

export { activatedFromSleep }
